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

#include "pqwriter.h"
#include "output-message.h"

#include "errorcodes.h"
#include "utilprotobuf.h"

#include "pkt2optionscache.h"

#include "messagedecomposer.h"
#include "utilstring.h"
#include "fieldnamevalueindexstrings.h"

using namespace google::protobuf;

int format_number;

/**
 * @brief MessageDecomposer callback. Use in conjunction with FieldNameValueIndexStrings class(see first parameter).
 * @param env accumulates field names and values in the InsertSQLStrings object
 * @param message_descriptor message
 * @param field field descriptor
 * @param value pointer to the data
 * @param size size occupied by data
 * @param index index 
 * @see FieldNameValueIndexStrings
 */
void addFieldValueString
(
	MessageDecomposer *decomposer, 
	void *env,
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor *field,
	void* value,
	int size,
	int index
)
{
	pkt2::Variable variable = field->options().GetExtension(pkt2::variable);
	((FieldNameValueIndexStrings *) env)->add(field, decomposer->toString(message_descriptor, field, value, size, format_number), index, true);
}

#define CHECK_STMT(error_message) \
		if (PQresultStatus(res) != PGRES_COMMAND_OK) \
		{ \
			LOG(ERROR) << ERR_DATABASE_STATEMENT_FAIL << error_message; \
			PQclear(res); \
			PQfinish(conn); \
			return ERRCODE_DATABASE_STATEMENT_FAIL; \
		}

int execSQL
(
		Config *config,
		const std::vector<std::string> &stmts
)
{
	PGconn *conn = dbconnect(config);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		LOG(ERROR) << ERR_DATABASE_NO_CONNECTION;
		return ERRCODE_DATABASE_NO_CONNECTION;
	}
	PGresult *res;
	res = PQexec(conn, "BEGIN");
	CHECK_STMT("start transaction")
	for (int i = 0; i < stmts.size(); i++)
	{
		res = PQexec(conn, stmts[i].c_str());
		CHECK_STMT(stmts[i])
	}
	res = PQexec(conn, "END");
	CHECK_STMT("commit transaction")

	PQfinish(conn);
	return 0;
}

/**
 * @brief Write line loop
 * @param config config
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

	format_number = config->format_number;

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

	if (config->verbosity > 2)
	{
		// print out create statements
		LOG(INFO) << "SQL CREATE TABLE statements";
		LOG(INFO) << "===========================";
		LOG(INFO) << "CREATE TABLE num (message VARCHAR(255), time INTEGER, device INTEGER, field VARCHAR(255), value NUMERIC(10, 2));";
		LOG(INFO) << "CREATE TABLE str (message VARCHAR(255), time INTEGER, device INTEGER, field VARCHAR(255), value VARCHAR(255));";
	}

	std::vector<std::string> clauses;
	pd.getStatementSQLCreate(&clauses, 0);
	if (config->verbosity > 2)
	{
		for (std::vector<std::string>::const_iterator it(clauses.begin()); it != clauses.end(); ++it)
			LOG(INFO) << *it;
	}

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

	std::vector<std::string> stmts;
	stmts.reserve(100);
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

			FieldNameValueIndexStrings vals(&options, messageTypeNAddress.message_type);
			
			MessageDecomposer md(&vals, messageTypeNAddress.message_type, &options, m, addFieldValueString);
			if (config->verbosity > 2)
			{
				LOG(INFO) << MSG_MESSAGE_JSON << vals.toStringJSON(&config->tableAliases, &config->fieldAliases);
			}
			
			stmts.clear();
			switch (config->mode)
			{
			case MODE_SQL:
				vals.toStringInsert(&stmts, &config->tableAliases, &config->fieldAliases);
				break;
			case MODE_SQL2:
				vals.toStringInsert2(&stmts, &config->tableAliases, &config->fieldAliases);
				break;
			default:
				break;
			}
			if (config->verbosity > 2)
			{
				std::stringstream ss;
				ss << std::endl;
				for (std::vector<std::string>::const_iterator it (stmts.begin()); it != stmts.end(); ++it)
				{
					ss << *it << std::endl;
				}
				LOG(INFO) << MSG_SQL_LINES << ss.str();
			}

			execSQL(config, stmts);
    	}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE;
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
