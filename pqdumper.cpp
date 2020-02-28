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

#include "pqdumper.h"

#include "errorcodes.h"
#include "utilstring.h"
#include "utilprotobuf.h"
#include "input-packet.h"

#define SQL_POSTGRESQL_DROP "DROP TABLE IF EXISTS packet CASCADE;";
#define SQL_POSTGRESQL_CREATE "CREATE TABLE packet (id BIGSERIAL PRIMARY KEY, tag INTEGER NOT NULL, \
	time TIMESTAMP WITH TIME ZONE NOT NULL, value TEXT NOT NULL, \
	src VARCHAR(16), srcport INTEGER, dst VARCHAR(16), dstport INTEGER);"
#define SQL_POSTGRESQL_INSERT_PREFIX "INSERT INTO \"packet\" (tag, time, \
	value, src, srcport, dst, dstport) VALUES(0, now(), "

using namespace google::protobuf;

#define CHECK_STMT(error_message) \
		if (PQresultStatus(res) != PGRES_COMMAND_OK) \
		{ \
			LOG(ERROR) << ERR_DATABASE_STATEMENT_FAIL << error_message << ": " << PQerrorMessage(conn); \
			PQclear(res); \
			PQfinish(conn); \
			return ERRCODE_DATABASE_STATEMENT_FAIL; \
		}

int execSQL
(
	Config *config,
	const std::string &stmt
)
{
	PGconn *conn = dbconnect(config);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		LOG(ERROR) << ERR_DATABASE_NO_CONNECTION;
		return ERRCODE_DATABASE_NO_CONNECTION;
	}
	PGresult *res;
	// res = PQexec(conn, "BEGIN");
	// CHECK_STMT("start transaction")
	res = PQexec(conn, stmt.c_str());
	CHECK_STMT(stmt)
	// res = PQexec(conn, "END");
	// CHECK_STMT("commit transaction")
	PQfinish(conn);
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
	if (config->create_table)
	{
		execSQL(config, SQL_POSTGRESQL_CREATE);
		return  ERR_OK;
	}
	if (config->verbosity > 2)
	{
		std::cerr << "Database host: " << config->dbhost 
			<< " port: " << config->dbport 
			<< " user: " << config->dbuser << " db: " << config->dbname << std::endl;
	}
	
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

	if (config->verbosity > 2) {
		LOG(INFO) << MSG_CONNECTED_TO << config->packet_url << ", buffer size: " << config->buffer_size;
	}

	// print out create statements
	LOG(INFO) << "SQL CREATE TABLE statements";
	LOG(INFO) << "===========================";
	LOG(INFO) << SQL_POSTGRESQL_DROP;
	LOG(INFO) << SQL_POSTGRESQL_CREATE;
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

/*		
		if (config->verbosity > 2) 
		{
			std::cerr << "Recieve: " << hexString(packet.data(), packet.length) << ", size " << payload_size;
		}
*/
		std::stringstream ss;
		ss << SQL_POSTGRESQL_INSERT_PREFIX << "'" << pkt2utilstring::hexString(packet.data(), packet.length) << "', '" 
			<< inet_ntoa(packet.get_sockaddr_src()->sin_addr) << "', " << ntohs(packet.get_sockaddr_src()->sin_port) << ", '" 
			<< inet_ntoa(packet.get_sockaddr_dst()->sin_addr) << "', " << ntohs(packet.get_sockaddr_dst()->sin_port) << ");";
		execSQL(config, ss.str());
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
