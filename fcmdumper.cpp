/**
 *
 */
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

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
#include "pg-connect.h"

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
	const std::string &imei,
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
	
	PGconn *conn = dbconnect(&config->pgconnect);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		LOG(ERROR) << ERR_DATABASE_NO_CONNECTION;
		return ERRCODE_DATABASE_NO_CONNECTION;
	}
	PGresult *res = PQexec(conn, "BEGIN");
	CHECK_STMT("start transaction")
	
	// get name
	std::string q = "SELECT device_description.device_name \
		FROM device_description, devices \
		WHERE device_description.imei = devices.id \
		AND device_description.current = 't' AND devices.imei = '" +  imei + "'";
	res = PQexec(conn, q.c_str());
	if ((PQresultStatus(res) != PGRES_TUPLES_OK) || (PQntuples(res) <= 0))
	{
		PQclear(res);
		PQfinish(conn);
		return ERRCODE_DATABASE_STATEMENT_FAIL;
	}
	std::string name = std::string(PQgetvalue(res, 0, 0));
	PQclear(res);
	
	// get root tokens
	q = "SELECT dev.instance FROM dev where dev.userid = 1";
	res = PQexec(conn, q.c_str());
	if (PQresultStatus(res) == PGRES_TUPLES_OK) 
	{
		int nrows = PQntuples(res);
		for (int row = 0; row < nrows; row++)
		{
			std::pair<std::string, std::string> p;
			p.first = std::string(PQgetvalue(res, row, 0));
			p.second = name;
			retval.push_back(p);
		}
		PQclear(res);
	}
	
	// get owner tokens
	q = "SELECT dev.instance \
		FROM device_description, dev, devices \
		WHERE dev.userid = device_description.owner \
		AND device_description.imei = devices.id \
		AND device_description.current = 't' AND devices.imei = '" +  imei + "'";
	res = PQexec(conn, q.c_str());

	if (PQresultStatus(res) == PGRES_TUPLES_OK) 
	{
		int nrows = PQntuples(res);
		for(int row = 0; row < nrows; row++)
		{
			std::pair<std::string, std::string> p;
			p.first = std::string(PQgetvalue(res, row, 0));
			p.second = name;
			retval.push_back(p);
		}
		PQclear(res);
	}

    res = PQexec(conn, "END");
	CHECK_STMT("commit transaction")
	PQclear(res);
	PQfinish(conn);
	return 0;
}

/**
 * @brief Send notification to the mobile device
 * @param buffer data to send as hex string
 * @param bytes size
 * @param config Configuration
 */
static int sendNotifications
(
	const void *buffer, 
	size_t bytes, 
	Config *config
)
{
	int c = 200;
	if ((!buffer) || (bytes <= 0)
		|| (bytes <= (config->imei_field_offset + config->imei_field_size)))
		return c;
	
	TokenNNameList tokens;
	
	// get IMEI from the message
	std::string imei((const char*) buffer + config->imei_field_offset, config->imei_field_size);
		
	getTokenNNameList(tokens, imei, bytes, config);
	if (config->verbosity > 1) 
	{
		std::cerr << "Send to: " << imei << ", key: " << config->server_key << std::endl
		 << "tokens: " << tokens.size() << " total" << std::endl;
	}

	std::string payload = pkt2utilstring::hexString(buffer, bytes);
	for (TokenNNameList::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
	{
		std::string r;
		c = push2instance(r, config->fburl, config->server_key, it->first, it->second, config->timezone, payload);

		if (config->verbosity > 2) 
		{

			std::cerr << it->first << " " << it->second << " code: " << c << std::endl;
		}

		if ((c > 299) || (c < 200))
			break;
	}
	return ((c < 300) && (c >199)) ? ERR_OK : ERRCODE_FIREBASE_WRITE;
}

/**
 * @brief Send notification to the mobile device
 * @param hex hex string
 * @param config Configuration
 */
int sendNotificationsHex
(
	const std::string &hex, 
	Config *config
)
{
	std::string bin = pkt2utilstring::stringFromHex(hex);
	return sendNotifications(bin.c_str(), bin.size(), config);
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
	
	if (config->verbosity > 2) {
		LOG(INFO) << MSG_CONNECTED_TO << config->packet_url << ", buffer size: " << config->buffer_size;
	}
	
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
		
		sendNotifications(packet.data(), packet.length, config);

		if (config->buffer_size <= 0)
			nn_freemsg(buffer);
	}

	int r = nn_shutdown(accept_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->packet_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}

	close(accept_socket);
	accept_socket = 0;

	free(buffer);
	buffer = NULL;

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
