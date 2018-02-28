/**
 * @file fcm-writer.cpp
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

#include <curl/curl.h>
#include <postgresql/libpq-fe.h>

#include "fcm-writer.h"
#include "output-message.h"

#include "errorcodes.h"
#include "utilprotobuf.h"
#include "handler-fcm-config.h"

#include "json/json.h"
#include "pbjson.hpp"

#include "pkt2optionscache.h"

#include "messagedecomposer.h"
#include "messageformat.h"
#include "utilstring.h"
#include "fieldnamevalueindexstrings.h"

using namespace google::protobuf;

/**
  * @brief CURL write callback
  */
static size_t write_string(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
* @brief Push notification to device
*/
int push2instance
(
	std::string &retval,
	const std::string &url,
	const std::string &serverkey,
	const std::string &client_token,
	const std::string &name,
	const std::string &data
)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return CURLE_FAILED_INIT; 
	std::string request_body = 
		"{\"to\": \"" + client_token + "\",\"data\":{" 
			+ "\"hex\": \"" + data
			+ "\", \"name\":\"" + name + "\"}}";
	
	struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
    chunk = curl_slist_append(chunk, std::string("Authorization: key=" + serverkey).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &retval);
	CURLcode res = curl_easy_perform(curl);
	int http_code;
    if (res != CURLE_OK)
	{
		retval = curl_easy_strerror(res);
		http_code = - res;
	}
	else
	{
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	}
	curl_easy_cleanup(curl);
	return http_code;
}

typedef std::vector<std::pair<std::string, std::string> > TokenNNameList;

#define CHECK_STMT(error_message) \
		if (PQresultStatus(res) != PGRES_COMMAND_OK) \
		{ \
			PQclear(res); \
			PQfinish(conn); \
			return ERRCODE_DATABASE_STATEMENT_FAIL; \
		}


int getTokenNNameList(
	TokenNNameList &retval,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	Config *config
) 
{
	// get IMEI from the message
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type, "\"", "\"");
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString);
	std::string imei = vals.findByLastName(config->imei_field_name);
	if (imei.empty())
		return 0;
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
	PQclear(res); \
	PQfinish(conn);

}

/**
 * @brief Print packet to the FireBase Cloud messaging
 * @param messageTypeNAddress message type
 * @param message message
 * @return 0 success
 */
int put
(
	Config *config,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	TokenNNameList tokenNames;
	if (getTokenNNameList(
		tokenNames, 
		options,
		messageTypeNAddress,
		message, 
		config) != 0)
		return ERRCODE_NO_NOBILE_SUBSCRIBERS;
	std::string s;
	pbjson::pb2json(message, s);
	int c;
	for (TokenNNameList::const_iterator it(tokenNames.begin()); it != tokenNames.end(); ++it)
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
 * @param config configuration
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
		LOG(ERROR) << ERR_NN_SUBSCRIBE << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SUBSCRIBE;
	}
	int eid = nn_connect(accept_socket, config->message_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd(config->proto_path, config->verbosity);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}
	Pkt2OptionsCache options(&pd);

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY;
		return ERRCODE_NO_MEMORY;
	}

	while (!config->stop_request)
    {
    	int bytes = nn_recv(accept_socket, buffer, config->buffer_size, 0);
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
		if (config->verbosity >= 2)
		{
			LOG(INFO) << MSG_RECEIVED << bytes << ": " << hexString(buffer, bytes) ;
		}

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

			put(config, &options, &messageTypeNAddress, m);
    	}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE << " " << hexString(buffer, bytes);
    }

    free(buffer);

	int r = nn_shutdown(accept_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url << " " << errno << " " << strerror(errno);
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
    {
    	LOG(ERROR) << ERR_STOP;
        return ERRCODE_STOP;
    }
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