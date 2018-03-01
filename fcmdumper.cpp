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
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "fcmdumper.h"
#include "errorcodes.h"
#include "utilstring.h"
#include "utilprotobuf.h"
#include "input-packet.h"

#include "helper_fcm.h"

#define SQL_POSTGRESQL_DROP "DROP TABLE IF EXISTS packet CASCADE;";
#define SQL_POSTGRESQL_CREATE "CREATE TABLE packet (id BIGSERIAL PRIMARY KEY, tag INTEGER NOT NULL, \
	time TIMESTAMP WITH TIME ZONE NOT NULL, value TEXT NOT NULL, \
	src VARCHAR(16), srcport INTEGER, dst VARCHAR(16), dstport INTEGER);"
#define SQL_POSTGRESQL_INSERT_PREFIX "INSERT INTO packet (tag, time, \
	value, src, srcport, dst, dstport) VALUES(0, now(), "

using namespace google::protobuf;

#define CHECK_STMT(error_message) \
	if (PQresultStatus(res) != PGRES_COMMAND_OK) \
	{ \
		LOG(ERROR) << ERR_DATABASE_STATEMENT_FAIL << error_message; \
		PQclear(res); \
		PQfinish(conn); \
		return ERRCODE_DATABASE_STATEMENT_FAIL; \
	}

//  Keep FireBase token and name of device
typedef std::vector<std::pair<std::string, std::string> > TokenNNameList;

/**
 * @brief Return FireBase token and name of device
 * @param retval Returns FireBase token and name of device
 * @param data binary packet data
 * @param config configuration
 * @return 0
 */
static int getTokenNNameList(
	TokenNNameList &retval,
	const void *data,
	size_t size,
	Config *config
) 
{
	if (config->packet_size > 0) 
	{
		// check size
		if (size < config->packet_size) 
		{
			return ERRCODE_DECOMPOSE_FATAL;
		}
	}

	if (config->imei_field_offset + config->imei_field_size >= size)
	{
		return ERRCODE_DECOMPOSE_FATAL;
	}
	
	// get IMEI from the message
	std::string imei((const char*) data + config->imei_field_offset, config->imei_field_size);
	
	// read FireBase tokens from the database
	std::string q = "SELECT dev.instance, device_description.device_name FROM device_description, dev WHERE dev.userid = device_description.owner \
	AND device_description.current = 't' AND device_description.imei = '" +  imei + "'";

	PGconn *conn = dbconnect(config);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		LOG(ERROR) << ERR_DATABASE_NO_CONNECTION;
		return ERRCODE_DATABASE_NO_CONNECTION;
	}
	PGresult *res;
	res = PQexec(conn, "BEGIN");
	CHECK_STMT("start transaction")
	res = PQexec(conn, q.c_str());

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
			PQclear(res);
			PQfinish(conn);
			return ERRCODE_DATABASE_STATEMENT_FAIL;
	}

    int nrows = PQntuples(res);
    for(int row = 0; row < nrows; row++)
    {
		std::pair<std::string, std::string> p;
		p.first = std::string(PQgetvalue(res, row, 0));
		p.second = std::string(PQgetvalue(res, row, 1));
		retval.push_back(p);
    }

    res = PQexec(conn, "END");
	CHECK_STMT("commit transaction")
	PQclear(res);
	PQfinish(conn);
}

/**
 * @brief Send notification to the mobile device
 * @param hex hex string
 * @param config Configuration
 */
int sendNotifications
(
	const std::string hex, 
	Config *config
)
{
	int c = 200;
	TokenNNameList tokens;
	
	getTokenNNameList(tokens, hex.c_str(), hex.size(), config);
	for (TokenNNameList::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
	{
		std::string r;
		c = push2instance(r, config->fburl, config->server_key, it->first, it->second, hex);
		if ((c > 299) || (c < 200))
			break;
	}
	return ((c < 300) && (c >199)) ? ERR_OK : ERRCODE_FIREBASE_WRITE;
}

/**
 * @brief Send notification to the mobile device
 * @param buffer data to send as hex string
 * @param bytes size
 * @param config Configuration
 */
int sendNotifications
(
	void *buffer, 
	size_t bytes, 
	Config *config
)
{
	int c = 200;
	TokenNNameList tokens;
	
	std::string s(hexString(buffer, bytes)); 
	getTokenNNameList(tokens, buffer, bytes, config);
	for (TokenNNameList::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
	{
		std::string r;
		c = push2instance(r, config->fburl, config->server_key, it->first, it->second, s);
		if ((c > 299) || (c < 200))
			break;
	}
	return ((c < 300) && (c >199)) ? ERR_OK : ERRCODE_FIREBASE_WRITE;
}

/**
 * @brief Write line loop
 * @param config Configuration
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

	int accept_socket = nn_socket(AF_SP, NN_BUS);
	// int r = nn_setsockopt(accept_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (accept_socket < 0)
	{
		LOG(ERROR) << ERR_NN_SUBSCRIBE << config->packet_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SUBSCRIBE;
	}
	int eid = nn_connect(accept_socket, config->packet_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->packet_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	std::vector<std::string> clauses;
	
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
		
		int payload_size = InputPacket::getPayloadSize(bytes);
        if (payload_size < 0)
        {
        	LOG(ERROR) << ERR_NN_RECV << " too small " << payload_size;
            continue;
        }
		InputPacket packet(buffer, bytes);

		if (packet.error() != 0)
		{
			LOG(ERROR) << ERRCODE_PACKET_PARSE << packet.error();
			continue;
		}

		sendNotifications(buffer, bytes, config);

		if (config->buffer_size <= 0)
			nn_freemsg(buffer);
	}

	free(buffer);

	int r = nn_shutdown(accept_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->packet_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}

	close(accept_socket);
	accept_socket = 0;

	if (config->stop_request == 2)
		goto START;

	return r;
}

/**
 *  @brief Stop writer
 *  @param config configuration
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
