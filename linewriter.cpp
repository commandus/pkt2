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

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "linewriter.h"
#include "output-message.h"

#include "errorcodes.h"
#include "utilprotobuf.h"

#include "json/json.h"
#include "pbjson.hpp"

#include "pkt2packetvariable.cpp"
#include "pkt2optionscache.h"

#include "messagedecomposer.h"
#include "utilstring.h"

using namespace google::protobuf;

/**
 * @brief Print packet to the stdout
 * @param messageTypeNAddress
 * @param message
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
 * @brief Print packet to the stdout as JSON.
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_json
(
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	std::string out;
	pbjson::pb2json(message, out);
	std::cout << out << std::endl;
	return ERR_OK;
}

/**
 * @brief Accumulate field names and values as string
 */
class FieldNameValueStrings
{
private:
	std::string table;
	std::vector<std::string> fields;
	std::vector<std::string> values;
	const std::string string_quote;
	std::string quote;
public:
	FieldNameValueStrings(const std::string &table_name)
		: table(table_name), string_quote ("'"), quote("\"")
	{

	};

	FieldNameValueStrings(
			const std::string &table_name,
			const std::string &astring_quote,
			const std::string &aquote
	)
		: table(table_name), string_quote (astring_quote), quote(aquote)
	{

	};

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	std::string toStringInsert() {
		std::stringstream ss;
		int sz = fields.size();
		ss << "INSERT INTO " << quote << table << quote << "(";
		for (int i = 0; i < sz; i++)
		{
			ss << quote << fields[i] << quote;
			if (i < sz - 1)
				ss << ", ";
		}
		ss << ") VALUES (";
		sz = values.size();
		for (int i = 0; i < sz; i++)
		{
			ss << values[i];
			if (i < sz - 1)
				ss << ", ";
		}
		ss << ");" << std::endl;
		return ss.str();
	};

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	std::string toStringInsert2() {
		std::stringstream ss;
		int sz = fields.size();
		ss << "INSERT INTO " << quote << table << quote << "(";
		for (int i = 0; i < sz; i++)
		{
			ss << quote << fields[i] << quote;
			if (i < sz - 1)
				ss << ", ";
		}
		ss << ") VALUES (";
		sz = values.size();
		for (int i = 0; i < sz; i++)
		{
			ss << values[i];
			if (i < sz - 1)
				ss << ", ";
		}
		ss << ");" << std::endl;
		return ss.str();
	};

	/**
	 * CSV line
	 * @return String
	 */
	std::string toStringCSV() {
		std::stringstream ss;
		int sz = fields.size();
		ss << quote << table << quote << ",";
		sz = values.size();
		for (int i = 0; i < sz; i++)
		{
			ss << values[i];
			if (i < sz - 1)
				ss << ", ";
		}
		ss << ";" << std::endl;
		return ss.str();
	};

	/**
	 * Tab delimited line
	 * @return String
	 */
	std::string toStringTab() {
		std::stringstream ss;
		int sz = fields.size();
		ss << quote << table << quote << "\t";
		sz = values.size();
		for (int i = 0; i < sz; i++)
		{
			ss << values[i];
			if (i < sz - 1)
				ss << "\t";
		}
		ss << std::endl;
		return ss.str();
	};

	inline void add
	(
		const std::string &field,
		const std::string &value
	)
	{
		fields.push_back(field);
		values.push_back(value);
	};

	inline void add_string
	(
		const std::string &field,
		const std::string &value
	)
	{
		std::string val(value);
		const std::string esc = string_quote + string_quote;
		replace(val, string_quote, esc);
		add(field, string_quote + field + string_quote);
	};

};

/**
 * @brief MessageDecomposer callback. Use in conjunction with FieldNameValueStrings class(see first parameter).
 * @param env accumulates field names and values in the InsertSQLStrings object
 * @param message_descriptor message
 * @param field_type type of the data
 * @param field_name field name
 * @param value pointer to the data
 * @param size  size occupied by data
 *
 * @see FieldNameValueStrings
 */
void addFieldValueString
(
	void *env,
	const google::protobuf::Descriptor *message_descriptor,
	google::protobuf::FieldDescriptor::CppType field_type,
	const std::string &field_name,
	void* value,
	int size
)
{
	FieldNameValueStrings *sqls = (FieldNameValueStrings *) env;
	if (field_type == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
		sqls->add_string(field_name, std::string((char *)value, size));
	else
		sqls->add(field_name, MessageDecomposer::toString(field_type, value, size));
}

/**
 * @brief Print packet to the stdout as SQL
 * @param messageTypeNAddress
 * @param message
 * @return
 */
int put_sql
(
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	FieldNameValueStrings vals(messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, message, addFieldValueString);
	std::cout << vals.toStringInsert();
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as SQL (2)
 * @param messageTypeNAddress
 * @param message
 * @return
 */
int put_sql2
(
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	FieldNameValueStrings vals(messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, message, addFieldValueString);
	std::cout << vals.toStringInsert2();
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as CSV
 * @param messageTypeNAddress
 * @param message
 * @return
 */
int put_csv
(
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	FieldNameValueStrings vals(messageTypeNAddress->message_type, "\"", "\"");
	MessageDecomposer md(&vals, message, addFieldValueString);
	std::cout << vals.toStringCSV();
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as CSV
 * @param messageTypeNAddress
 * @param message
 * @return
 */
int put_tab
(
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	FieldNameValueStrings vals(messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, message, addFieldValueString);
	std::cout << vals.toStringTab();
	return ERR_OK;
}

/**
 * @brief Print packet's message options to the stdout
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_pkt2_options
(
		ProtobufDeclarations *pd,
		MessageTypeNAddress *messageTypeNAddress
)
{
	// Each message
	const google::protobuf::Descriptor* md = pd->getMessageDescriptor(messageTypeNAddress->message_type);
	if (!md)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << messageTypeNAddress->message_type;
		return ERRCODE_MESSAGE_TYPE_NOT_FOUND;
	}

	const google::protobuf::MessageOptions options = md->options();
	if (options.HasExtension(pkt2::packet))
	{
		std::string out;
		pkt2::Packet packet =  options.GetExtension(pkt2::packet);
		pbjson::pb2json(&packet, out);
		std::cout << "{\"packet\":" << out << "," << std::endl;
	}

	std::cout << "\"variables\":[";
	for (int f = 0; f < md->field_count(); f++)
	{
		const google::protobuf::FieldOptions foptions = md->field(f)->options();
		std::string out;
		pkt2::Variable variable = foptions.GetExtension(pkt2::variable);
		pbjson::pb2json(&variable, out);
		std::cout << out;
		if (f < md->field_count() - 1)
			std::cout << ",";
	}
	std::cout << "]}" << std::endl;

	return ERR_OK;
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

	ProtobufDeclarations pd(config->proto_path);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY;
		return ERRCODE_NO_MEMORY;
	}

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
			switch (config->mode)
			{
			case MODE_JSON:
				put_json(&messageTypeNAddress, m);
				break;
			case MODE_CSV:
				put_csv(&messageTypeNAddress, m);
				break;
			case MODE_TAB:
				put_tab(&messageTypeNAddress, m);
				break;
			case MODE_SQL:
				put_sql(&messageTypeNAddress, m);
				break;
			case MODE_SQL2:
				put_sql2(&messageTypeNAddress, m);
				break;
			case MODE_OPTIONS:
				put_pkt2_options(&pd, &messageTypeNAddress);
				break;
			default:
				put_debug(&messageTypeNAddress, m);
			}
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
