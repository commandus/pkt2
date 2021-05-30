/**
 *	Write Protobuf message to the LMDB database
 */
#include <algorithm>
#include <string.h>
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
#include <Winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <sys/types.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "lmdbwriter.h"
#include "output-message.h"

#include "lmdb.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "pkt2packetvariable.h"
#include "pkt2optionscache.h"

using namespace google::protobuf;

/**
 * @brief LMDB environment(transaction, cursor)
 */
typedef struct dbenv {
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn *txn;
	MDB_cursor *cursor;

	// for re-init
	std::string path;
	int flags;
	int mode;
	int queue;
} dbenv;

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
 * @brief Opens LMDB database file
 * @param env created LMDB environment(transaction, cursor)
 * @param config pass path, flags, file open mode
 * @return true- success
 */
bool open_lmdb
(
	struct dbenv *env
)
{
	int rc = mdb_env_create(&env->env);
	if (rc)
	{
		LOG(ERROR) << "mdb_env_create error " << rc << " " << mdb_strerror(rc);
		env->env = NULL;
		return false;
	}

	rc = mdb_env_open(env->env, env->path.c_str(), env->flags, env->mode);
	if (rc)
	{
		LOG(ERROR) << "mdb_env_open path: " << env->path.c_str() << " error " << rc << " " << mdb_strerror(rc);
		env->env = NULL;
		return false;
	}

	rc = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (rc)
	{
		LOG(ERROR) << "mdb_txn_begin error " << rc << " " << mdb_strerror(rc);
		env->env = NULL;
		return false;
	}


	rc = mdb_open(env->txn, NULL, 0, &env->dbi);
	if (rc)
	{
		LOG(ERROR) << "mdb_open error " << rc << " " << mdb_strerror(rc);
		env->env = NULL;
		return false;
	}

	rc = mdb_txn_commit(env->txn);

	return rc == 0;
}

static int processMapFull
(
	dbenv *env
)
{
	mdb_txn_abort(env->txn);
	struct MDB_envinfo current_info;
	int r = mdb_env_info(env->env, &current_info);
	if (r)
	{
		LOG(ERROR) << "map full, mdb_env_info error " << r << ": " << mdb_strerror(r) << std::endl;
		return r;
	}
	if (!close_lmdb(env))
	{
		LOG(ERROR) << "map full, error close database " << std::endl;
		return ERRCODE_LMDB_CLOSE;
	}
	size_t new_size = current_info.me_mapsize * 2;
	LOG(INFO) << "map full, doubling map size from " << current_info.me_mapsize << " to " << new_size << " bytes" << std::endl;

	r = mdb_env_create(&env->env);
	if (r)
	{
		LOG(ERROR) << "map full, mdb_env_create error " << r << ": " << mdb_strerror(r) << std::endl;
		env->env = NULL;
		return ERRCODE_LMDB_OPEN;
	}
	r = mdb_env_set_mapsize(env->env, new_size);
	if (r)
		LOG(ERROR) << "map full, mdb_env_set_mapsize error " << r << ": " << mdb_strerror(r) << std::endl;
	r = mdb_env_open(env->env, env->path.c_str(), env->flags, env->mode);
	mdb_env_close(env->env);
	
	if (!open_lmdb(env))
	{
		LOG(ERROR) << "map full, error re-open database" << std::endl;
		return r;
	}

	// start transaction
	r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
		LOG(ERROR) << "map full, begin transaction error " << r << ": " << mdb_strerror(r) << std::endl;
	return r;
}

/**
 * @brief Store input packet to the LMDB
 * @param env database env
 * @param options   cache
 * @param buffer buffer
 * @param buffer_size buffer size
 * @param messageTypeNAddress address
 * @param message message
 * @return 0 - success
 */
int put_db
(
		struct dbenv *env,
		Pkt2OptionsCache *options,
		void *buffer,
		int buffer_size,
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	MDB_val key, data;
	char keybuffer[2048];

	key.mv_size = options->getKey(messageTypeNAddress->message_type, keybuffer, sizeof(keybuffer), message);
	if (key.mv_size == 0)
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
	{
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
			if (r == 0)
				r = mdb_put(env->txn, env->dbi, &key, &data, 0);
		}
		if (r)
		{
			mdb_txn_abort(env->txn);
			LOG(ERROR) << ERR_LMDB_PUT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_PUT;
		}
	}

	r = mdb_txn_commit(env->txn);
	if (r)
	{
		if (r == MDB_MAP_FULL) 
		{
			r = processMapFull(env);
		}
		if (r)
		{
			LOG(ERROR) << ERR_LMDB_TXN_COMMIT << r << ": " << mdb_strerror(r) << std::endl;
			return ERRCODE_LMDB_TXN_COMMIT;
		}
	}
	
	return r;
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
START:
	config->stop_request = 0;
	int accept_socket = nn_socket(AF_SP, NN_SUB);
	int r = nn_setsockopt(accept_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SUBSCRIBE << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SUBSCRIBE;
	}

	int eid = nn_connect(accept_socket, config->message_url.c_str());

	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url;
		return ERRCODE_NN_CONNECT;
	}

	struct dbenv env;
	env.path = config->path;
	env.flags = config->flags;
	env.mode = config->mode;
	env.queue = 0;

	if (!open_lmdb(&env))
	{
		LOG(ERROR) << ERR_LMDB_OPEN << config->path;
		return ERRCODE_LMDB_OPEN;
	}

	ProtobufDeclarations pd(config->proto_path, config->verbosity);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}
	Pkt2OptionsCache options(&pd);

	void *buffer;
	if (config->buffer_size > 0)
	{
		buffer = malloc(config->buffer_size);
	    if (!buffer)
		{
	    	LOG(ERROR) << ERR_NO_MEMORY;
	    	return ERRCODE_NO_MEMORY;
		}
	}
	else
		buffer = NULL;

    while (!config->stop_request)
    {
    	int bytes;
    	if (config->buffer_size > 0)
    		bytes = nn_recv(accept_socket, buffer, config->buffer_size, 0);
    	else
    		bytes = nn_recv(accept_socket, &buffer, NN_MSG, 0);

		
    	if (bytes < 0)
    	{
			if (errno == EINTR) 
			{
				LOG(ERROR) << ERR_INTERRUPTED;
				config->stop_request = true;
				break;
			}
			else
				LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, buffer, bytes, &messageTypeNAddress);
		if (m)
		{
			if (config->allowed_messages.size())
			{
				if (std::find(config->allowed_messages.begin(), config->allowed_messages.end(), m->GetTypeName()) == config->allowed_messages.end())
				{
					LOG(INFO) << MSG_PACKET_REJECTED << m->GetTypeName();
					continue;
				}
			}

			put_db(&env, &options, buffer, bytes, &messageTypeNAddress, m);
		}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE;

		if (config->buffer_size <= 0)
			nn_freemsg(buffer);
    }

	r = 0;

	if (!close_lmdb(&env))
	{
		LOG(ERROR) << ERR_LMDB_CLOSE << config->path;
		r = ERRCODE_LMDB_CLOSE;
	}

	r = nn_shutdown(accept_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url;
		r = ERRCODE_NN_SHUTDOWN;

	}

	close(accept_socket);
	accept_socket = 0;

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
        return ERRCODE_NO_CONFIG;
    config->stop_request = 1;
    return ERR_OK;
}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	return ERR_OK;
}
