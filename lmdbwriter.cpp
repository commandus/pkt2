/**
 *
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
#include "input-packet.h"
#include "output-message.h"

#include "lmdb.h"
#include "errorcodes.h"

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
 * Close LMDB database file
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
 * @brief Store protobuf message to the LMDB
 * @param env LMDB database
 * @return 0- success
 */
int put_db
(
		struct dbenv *env,
		struct OutputMessageKey *message_key,
		const google::protobuf::Message *message_value
)
{
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
		return r;

	MDB_val key, data;

	key.mv_size = sizeof(struct OutputMessageKey);
	key.mv_data = message_key;

	std::string s = message_value->SerializeAsString();
	data.mv_size = s.length();
	data.mv_data = &s;

	r = mdb_put(env->txn, env->dbi, &key, &data, 0);
	if (r)
		return r;

	r = mdb_txn_commit(env->txn);
	return r;
}


/**
 * @brief Store input packet to the LMDB
 * @param env
 * @param packet
 * @return 0 - success
 */
int put_db
(
		struct dbenv *env,
		InputPacket *packet
)
{
	int r = mdb_txn_begin(env->env, NULL, 0, &env->txn);
	if (r)
		return r;

	MDB_val key, data;

	google::protobuf::Message *m = packet->message;
	if (!m)
		return ERRCODE_PACKET_PARSE;

	uint32_t packet_type_id;
	uint32_t time_stamp;
	uint64_t device_id;

	key.mv_size = sizeof(struct OutputMessageKey);
	key.mv_data = &packet->key;

	data.mv_size = packet->data_size;
	data.mv_data = packet->data();

	r = mdb_put(env->txn, env->dbi, &key, &data, 0);

	delete m;

	if (r)
		return r;

	r = mdb_txn_commit(env->txn);
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
    while (!config->stop_request)
    {
        char *buf = NULL;
          int bytes = nn_recv(nano_socket, &buf, NN_MSG, 0);

          if (bytes < 0)
          {
              LOG(ERROR) << ERR_NN_RECV << bytes;
              continue;
          }
          if (buf)
          {
              InputPacket packet(buf, bytes);
              LOG(INFO) << "packet " << std::string(1, packet.header()->name) << " "
                  << packet.get_socket_addr_src() << " ";

              if (packet.error() != 0)
              {
                  LOG(ERROR) << ERR_PACKET_PARSE << packet.error();
                  continue;
              }
              put_db(&env, &packet);
              nn_freemsg(buf);
          }
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
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->path;
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
