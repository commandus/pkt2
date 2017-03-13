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

const std::string sql2table = "proto";
const std::string sql2names[] = {"message", "time", "device", "field"};

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

class FieldNameValueString
{
public:
	int index;
	const std::string field;
	const std::string value;
	FieldNameValueString(
			int idx,
			const std::string &fld,
			const std::string &val)
		: index(idx), field(fld), value(val)
	{

	};
};

/**
 * @brief Accumulate field names and values as string
 */
class FieldNameValueStrings
{
private:
	std::string table;
	std::vector<int> index2values;
	std::vector<FieldNameValueString> values;
	const std::string string_quote;
	std::string quote;
public:
	FieldNameValueStrings(const std::string &table_name)
		: table(table_name), string_quote ("'"), quote("\"")
	{
		index2values.resize(3, 0);
	};

	FieldNameValueStrings(
			const std::string &table_name,
			const std::string &astring_quote,
			const std::string &aquote
	)
		: table(table_name), string_quote (astring_quote), quote(aquote)
	{
		index2values.resize(3, 0);
	};

	/**
	 * After all message "parsed" get INSERT clause
	 * @return String
	 */
	std::string toStringInsert() {
		std::stringstream ss;
		int sz = values.size();
		ss << "INSERT INTO " << quote << replace(table, ".", "_") << quote << "(";
		for (int i = 0; i < sz; i++)
		{
			ss << quote << values[i].field << quote;
			if (i < sz - 1)
				ss << ", ";
		}
		ss << ") VALUES (";
		sz = values.size();
		for (int i = 0; i < sz; i++)
		{
			ss << values[i].value;
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
		std::stringstream sskey;
		sskey << "INSERT INTO " << quote << sql2table << quote << "(" << sql2names[0] << ",";

		// index first
		for (int i = 1; i < index2values.size(); i++)
		{
			if (!index2values[i])
				break;
			// sskey << values[index2values[i]].field << ",";
			sskey << sql2names[i] << ",";
		}
		sskey << sql2names[3] << ",";

		// non-index
		for (int i = 0; i < values.size(); i++)
		{
			if (index2values[i])
				continue;
			sskey << values[i].field << ",";
		}
		// remove last ","
		std::string key_prefix(sskey.str());
		key_prefix = key_prefix.substr(0, key_prefix.size() - 1);


		std::stringstream ssprefix;
		ssprefix << key_prefix << ") VALUES(";
		// ssprefix << string_quote << table << string_quote << ",";
		// values (index first)
		for (int i = 0; i < index2values.size(); i++)
		{
			if (!index2values[i])
				break;
			ssprefix << values[index2values[i]].value << ",";
		}

		std::string prefix(ssprefix.str());
		// prefix = prefix.substr(0, prefix.size() - 1); // remove last ","


		// each field
		std::stringstream ss;
		// non-index
		for (int i = 0; i < values.size(); i++)
		{
			if (index2values[i])
				continue;
			ss << prefix
				<< "," << values[i].value << ");" << std::endl;
		}

		return ss.str();
	};

	/**
	 * CSV line
	 * @return String
	 */
	std::string toStringCSV() {
		std::stringstream ss;
		int sz = values.size();
		ss << quote << table << quote << ",";
		sz = values.size();
		for (int i = 0; i < sz; i++)
		{
			ss << values[i].value;
			if (i < sz - 1)
				ss << ", ";
		}
		ss << std::endl;
		return ss.str();
	};

	/**
	 * Tab delimited line
	 * @return String
	 */
	std::string toStringTab() {
		std::stringstream ss;
		int sz = values.size();
		ss << quote << table << quote << "\t";
		for (int i = 0; i < sz; i++)
		{
			ss << values[i].value;
			if (i < sz - 1)
				ss << "\t";
		}
		ss << std::endl;
		return ss.str();
	};

	inline void add
	(
		const std::string &field,
		const std::string &value,
		int index
	)
	{
		if ((index > 0) && (index < index2values.size()))
			index2values[index] = values.size();
		values.push_back(FieldNameValueString(index, field, value));
	};

	inline void add_string
	(
		const std::string &field,
		const std::string &value,
		int index
	)
	{
		add(field, string_quote + replace(value, string_quote, string_quote + string_quote) + string_quote, index);
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
	int size,
	int index
)
{
	FieldNameValueStrings *sqls = (FieldNameValueStrings *) env;
	if (field_type == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
		sqls->add_string(field_name, std::string((char *)value, size), index);
	else
		sqls->add(field_name, MessageDecomposer::toString(field_type, value, size), index);
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
		Pkt2OptionsCache *options,
		const google::protobuf::Message *message
)
{
	FieldNameValueStrings vals(messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, options, message, addFieldValueString);
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
	Pkt2OptionsCache options(&pd);

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
				put_sql2(&messageTypeNAddress, &options, m);
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
