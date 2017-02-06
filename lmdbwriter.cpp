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

/**
 * LMDB environment(transaction, cursor)
 */
typedef struct dbenv {
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn *txn;
	MDB_cursor *cursor;
} dbenv;

/**
 * Opens LMDB database file
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
}

/**
 * @param env LMDB database
 * @returns 0- success
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
}

/**
  * Return:  0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open
  *          5- LMDB open database file error
  */
int run
(
		Config *config
)
{
	int nano_socket = nn_socket(AF_SP, NN_PUSH);
	if (nn_connect(nano_socket, config->message_url.c_str()) < 0)
	{
		LOG(ERROR) << "Can not connect to the IPC url " << config->message_url;
		return 2;
	}

	struct dbenv env;

	if (!open_lmdb(&env, config))
	{
		LOG(ERROR) << "Can not open database file " << config->path;
		return 5;
	}
    while (!config->stop_request)
    {
        char *buf = NULL;
          int bytes = nn_recv(nano_socket, &buf, NN_MSG, 0);

          if (bytes < 0)
          {
              LOG(ERROR) << "nn_recv error: " << bytes << ": ";
              continue;
          }
          if (buf)
          {
              InputPacket packet(buf, bytes);
              LOG(INFO) << "packet " << std::string(1, packet.header()->name) << " "
                  << packet.get_socket_addr_src() << " ";

              if (packet.error() != 0)
              {
                  LOG(ERROR) << "packet error: " << packet.error();
                  continue;
              }
              nn_freemsg(buf);
          }
    }
	if (!close_lmdb(&env))
	{
		LOG(ERROR) << "Can not close database file " << config->path;
	}
	return nn_shutdown(nano_socket, 0);
}

/**
  * Return 0- success
  *        1- config is not initialized yet
  */
int stop
(
		Config *config
)
{
    if (!config)
        return 1;
    config->stop_request = true;
    // wake up

}
