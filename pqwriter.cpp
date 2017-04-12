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
#include <nanomsg/pubsub.h>

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

/**
 * @brief MessageDecomposer callback. Use in conjunction with FieldNameValueIndexStrings class(see first parameter).
 * @param env accumulates field names and values in the InsertSQLStrings object
 * @param message_descriptor message
 * @param field_type type of the data
 * @param field_name field name
 * @param value pointer to the data
 * @param size  size occupied by data
 *
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
	((FieldNameValueIndexStrings *) env)->add(field->cpp_type(), field->name(), decomposer->toString(field, value, size), index);
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
}

/**
 * @brief Write line loop
 * @param config
 * @return  0- success
 *          >0- error (see errorcodes.h)
 */
int run
(
		Config *config
)
{
	int nano_socket = nn_socket(AF_SP, NN_SUB);
	int r = nn_setsockopt(nano_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SUBSCRIBE << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SUBSCRIBE;
	}
	int eid = nn_connect(nano_socket, config->message_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd(config->proto_path, config->verbosity);

	// print out create statements
	LOG(INFO) << "SQL CREATE TABLE statements";
	LOG(INFO) << "===========================";
	LOG(INFO) << "CREATE TABLE num (message VARCHAR(255), time INTEGER, device INTEGER, field VARCHAR(255), value NUMERIC(10, 2));";
	LOG(INFO) << "CREATE TABLE str (message VARCHAR(255), time INTEGER, device INTEGER, field VARCHAR(255), value VARCHAR(255));";

	std::vector<std::string> clauses;
	pd.getStatementSQLCreate(&clauses, 0);
	for (std::string &clause : clauses)
		LOG(INFO) << clause;

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

    	int bytes = nn_recv(nano_socket, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
    		LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, buffer, bytes, &messageTypeNAddress);
		if (m)
		{
			FieldNameValueIndexStrings vals(&options, messageTypeNAddress.message_type);
			MessageDecomposer md(&vals, &options, m, addFieldValueString);
			stmts.clear();
			switch (config->mode)
			{
			case MODE_SQL:
				vals.toStringInsert(&stmts);
				break;
			case MODE_SQL2:
				vals.toStringInsert2(&stmts);
				break;
			default:
				break;
			}
			execSQL(config, stmts);
    	}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE;
    }

    free(buffer);

	r = nn_shutdown(nano_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url << " " << errno << " " << strerror(errno);
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
