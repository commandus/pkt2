/**
 *
 */
#include <fstream>
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

#include "linewriter.h"
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
 * @brief Print message packet to the stdout
 * @param messageTypeNAddress type name, address
 * @param message message to print
 * @return 0 - success
 */
int put_debug
(
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	std::cout << message->DebugString() << std::endl;
	return 0;
}

/**
 * @brief MessageDecomposer callback. Use in conjunction with FieldNameValueIndexStrings class(see first parameter).
 * @param env accumulates field names and values in the InsertSQLStrings object
 * @param message_descriptor message
 * @param field field descriptor
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
	// std::cerr << field->cpp_type() << " " << field->name() << ": " << decomposer->toString(message_descriptor, field, value, size, format_number) << std::endl;
	((FieldNameValueIndexStrings *) env)->add(field, decomposer->toString(message_descriptor, field, value, size, format_number), index);
}

/**
 * @brief Print packet to the stdout as CSV
 * @return 0
 */
int put_json
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, options, message, addFieldValueString);
	*output << vals.toStringJSON();
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as SQL
 * @param messageTypeNAddress message full type name, IP source and destination addresses
 * @param message message Protobuf message
 * @return 0- success
 */
int put_sql
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, options, message, addFieldValueString);
	std::vector<std::string> stmts;
	vals.toStringInsert(&stmts);
	for (std::vector<std::string>::const_iterator it(stmts.begin()); it != stmts.end(); ++it)
		*output << *it;
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as SQL (2)
 * @param messageTypeNAddress type and address
 * @param message message
 * @return
 */
int put_sql2
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, options, message, addFieldValueString);
	std::vector<std::string> stmts;
	vals.toStringInsert2(&stmts);
	for (std::vector<std::string>::const_iterator it(stmts.begin()); it != stmts.end(); ++it)
		*output << *it;
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as CSV
 * @param options Pkt2OptionsCache 
 * @param messageTypeNAddress type and address
 * @param message message
 * @return
 */
int put_csv
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type, "\"", "\"");
	MessageDecomposer md(&vals, options, message, addFieldValueString);
	*output << vals.toStringCSV();
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as CSV
 * @param messageTypeNAddress type and address
 * @param message message
 * @return
 */
int put_tab
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, options, message, addFieldValueString);
	*output << vals.toStringTab();
	return ERR_OK;
}

/**
 * @brief Write line loop
 * @param config program configuration
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
	
	if (!config->file_name.empty())
		config->stream = new std::ofstream(config->file_name.c_str(), std::ofstream::app);
	else
		config->stream = &std::cout;
	
	config->accept_socket = nn_socket(AF_SP, NN_BUS);
	// int r = nn_setsockopt(config->accept_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (config->accept_socket < 0)
	{
		LOG(ERROR) << ERR_NN_SOCKET << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SOCKET;
	}
	int eid = nn_connect(config->accept_socket, config->message_url.c_str());
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

    	int bytes = nn_recv(config->accept_socket, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
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

			switch (config->mode)
			{
			case MODE_JSON:
				put_json(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_CSV:
				put_csv(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_TAB:
				put_tab(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_SQL:
				put_sql(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_SQL2:
				put_sql2(config->stream, &options, &messageTypeNAddress, m);
				break;
			default:
				put_debug(&messageTypeNAddress, m);
			}
    	}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE << " " << hexString(buffer, bytes);
    }

    free(buffer);
	if (!config->file_name.empty())
	{
		if (config->stream)
		{
			delete config->stream;
			config->stream = NULL;
		}
	}

	int r = nn_shutdown(config->accept_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}
	
	if (config->stop_request == 2)
		goto START;

	return r;
}

/**
 *  @brief Stop writer
 *  @param config program config
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
	close(config->accept_socket);
	config->accept_socket = 0;
	return ERR_OK;
}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	close(config->accept_socket);
	config->accept_socket = 0;
	return ERR_OK;
}
