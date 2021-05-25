#include "messageformat.h"

#include "fieldnamevalueindexstrings.h"

static int format_number;

void set_format_number(int value)
{
	format_number = value;
}
	
/**
 * @brief Print message packet to the stdout
 * @param output stream
 * @param messageTypeNAddress type name, address
 * @param message message to print
 * @return 0 - success
 */
int put_debug
(
	std::ostream *output,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	*output << message->DebugString() << std::endl;
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
	((FieldNameValueIndexStrings *) env)->add(field, decomposer->toString(message_descriptor, field, value, size, format_number), index, false);
}

/**
 * @brief Print packet to the stdout as JSON
 * @return 0
 */
int put_json
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString);
	*output << vals.toStringJSON(tableAliases, fieldAliases);
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
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString);
	std::vector<std::string> stmts;
	vals.toStringInsert(&stmts, tableAliases, fieldAliases);
	for (std::vector<std::string>::const_iterator it(stmts.begin()); it != stmts.end(); ++it)
		*output << *it;
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as SQL (2)
 * @param messageTypeNAddress type and address
 * @param message message
 * @return 0
 */
int put_sql2
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString);
	std::vector<std::string> stmts;
	vals.toStringInsert2(&stmts, tableAliases, fieldAliases);
	for (std::vector<std::string>::const_iterator it(stmts.begin()); it != stmts.end(); ++it)
		*output << *it;
	return ERR_OK;
}

/**
 * @brief Print packet to the stdout as Protobuf text
 * @param messageTypeNAddress type and address
 * @param message message
 * @return 0
 */
int put_protobuf_text
(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	*output << message->SerializeAsString();
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
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString);
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
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString);
	*output << vals.toStringTab();
	return ERR_OK;
}

/**
 * @brief return field names
 * @return 0 if message type is unknown
 */
std::string getFieldNames
(
	std::vector <std::string> &retval,
	Pkt2OptionsCache *options,
	const std::string &messageTypeName
)
{
	bool found;
	const Pkt2PacketVariable &v(options->getPacketVariable(messageTypeName, &found));
	if (!found)
		return 0;
	for (std::vector<FieldNameVariable>::const_iterator it = v.fieldname_variables.begin(); it != v.fieldname_variables.end(); it++) {
		retval.push_back(v.message_type + "." + it->field_name);
	}
	return v.message_type;
}
