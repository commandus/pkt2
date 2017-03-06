/**
 *	Write Protobuf message to the LMDB database
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "lmdbwriter.h"
#include "output-message.h"

#include "lmdb.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "pkt2packetvariable.h"

using namespace google::protobuf;

/**
 * @brief LMDB environment(transaction, cursor)
 */
typedef struct dbenv {
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn *txn;
	MDB_cursor *cursor;
} dbenv;

/**
 * @brief Opens LMDB database file
 * @param env created LMDB environment(transaction, cursor)
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool open_lmdb
(
	struct dbenv *env,
	Config *config
)
{
	int rc = mdb_env_create(&env->env);
	rc |= mdb_env_open(env->env, config->path.c_str(), config->flags, config->mode);
	rc |= mdb_open(env->txn, NULL, 0, &env->dbi);
	return rc == 0;
}

/**
 * @brief Close LMDB database file
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool close_lmdb
(
	struct dbenv *env
)
{
	mdb_close(env->env, env->dbi);
	mdb_env_close(env->env);
	return true;
}

/**
 * @brief Store input packet to the LMDB
 * @param env
 * @param buffer
 * @param buffer_size
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_db
(
		struct dbenv *env,
		void *buffer,
		int buffer_size,
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	MDB_val key, data;
	char keybuffer[2048];

	key.mv_size = messageTypeNAddress->getKey(keybuffer, sizeof(keybuffer), message);
	if (key.mv_size)
		return 0;	// No key, no store

	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_BEGIN << r;
		return ERRCODE_LMDB_TXN_BEGIN;
	}

	key.mv_data = keybuffer;

	data.mv_size = buffer_size;
	data.mv_data = buffer;

	r = mdb_put(env->txn, env->dbi, &key, &data, 0);

	if (r)
		return r;

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r;
		return ERRCODE_LMDB_TXN_COMMIT;
	}
	return r;
}

/**
 * @brief get packet's message options
 * @param buffer
 * @param buffer_size
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_pkt2_options
(
		void *buffer,
		int buffer_size,
		ProtobufDeclarations *pd,
		MessageTypeNAddress *messageTypeNAddress
)
{
	// Each message
	const google::protobuf::Descriptor* md = pd->getMessageDescriptor(messageTypeNAddress->message_type);
	if (!md)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << messageTypeNAddress->message_type;
		return ERRCODE_MESSAGE_TYPE_NOT_FOUND;
	}

	const google::protobuf::MessageOptions options = md->options();
	if (options.HasExtension(pkt2::packet))
	{
		std::string out;
		pkt2::Packet packet =  options.GetExtension(pkt2::packet);
	}

	for (int f = 0; f < md->field_count(); f++)
	{
		const google::protobuf::FieldOptions foptions = md->field(f)->options();
		std::string out;
		pkt2::Variable variable = foptions.GetExtension(pkt2::variable);
	}

	return ERR_OK;
}

/**
 * @brief Write LMDB loop
 * @param config
 * @return  0- success
 *          >0- error (see errorcodes.h)
 */
int run
(
		Config *config
)
{
	int nano_socket = nn_socket(AF_SP, NN_PUSH);
	if (nn_connect(nano_socket, config->message_url.c_str()) < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url;
		return ERRCODE_NN_CONNECT;
	}

	struct dbenv env;

	if (!open_lmdb(&env, config))
	{
		LOG(ERROR) << ERR_LMDB_OPEN << config->path;
		return ERRCODE_LMDB_OPEN;
	}

	ProtobufDeclarations pd(config->proto_path);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

    while (!config->stop_request)
    {
    	char *buffer = NULL;
    	int bytes = nn_recv(nano_socket, &buffer, NN_MSG, 0);

    	if (bytes < 0)
    	{
    		LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, buffer, bytes, &messageTypeNAddress);
		if (m)
			put_db(&env, buffer, bytes, &messageTypeNAddress, m);
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE;

		nn_freemsg(buffer);
    }

	int r = 0;

	if (!close_lmdb(&env))
	{
		LOG(ERROR) << ERR_LMDB_CLOSE << config->path;
		r = ERRCODE_LMDB_CLOSE;
	}

	r = nn_shutdown(nano_socket, 0);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url;
		r = ERRCODE_NN_SHUTDOWN;

	}
	return r;
}

/**
 *  @brief Stop writer
 *  @param config
 *  @return 0- success
 *          >0- config is not initialized yet
 */
int stop
(
		Config *config
)
{
    if (!config)
    {
    	LOG(ERROR) << ERR_STOP;
        return ERRCODE_STOP;
    }
    config->stop_request = true;
    // wake up
    return ERR_OK;
}
