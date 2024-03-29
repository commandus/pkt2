#ifndef MESSAGE_FORMAT_H_
#define MESSAGE_FORMAT_H_

#include <map>

#include "packet2message.h"
#include "utilprotobuf.h"
#include "messagedecomposer.h"
#include "fieldnamevalueindexstrings.h"

// output modes
#define MODE_JSON			0
#define MODE_CSV			1
#define MODE_TAB			2
#define MODE_SQL			3
#define MODE_SQL2			4
#define MODE_PB_TEXT		5
#define MODE_PRINT_DBG		6

// Copy decraration from pkt2.proto
enum SQL_DIALECT : int {
	SQL_POSTGRESQL = 0,
	SQL_MYSQL = 1,
	SQL_FIREBIRD = 2,
	SQL_SQLITE = 3
};
  
void set_format_number(
	int value
);

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
);

/**
 * @brief Print packet to the stdout as JSON
 * @return 0
 */
int put_json(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
);

/**
 * @brief Print packet to the stdout as SQL
 * @param messageTypeNAddress message full type name, IP source and destination addresses
 * @param message message Protobuf message
 * @return 0- success
 */
int put_sql(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties,
	int sqlDialect
);

/**
 * @brief Print packet to the stdout as SQL (2)
 * @param messageTypeNAddress type and address
 * @param message message
 * @return 0- success
 */
int put_sql2(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,\
	const std::map<std::string, std::string> *properties,
	int sqlDialect
);

/**
 * @brief Print packet to the stdout as CSV
 * @param options Pkt2OptionsCache 
 * @param messageTypeNAddress type and address
 * @param message message
 * @return 0- success
 */
int put_csv(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
);

/**
 * @brief Print packet to the stdout as CSV
 * @param messageTypeNAddress type and address
 * @param message message
 * @return 0- success
 */
int put_tab(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
);

/**
 * @brief Print packet to the stdout as Protobuf text
 * @param messageTypeNAddress type and address
 * @param message message
 * @return 0
 */
int put_protobuf_text(
	std::ostream *output,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
);

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
void addFieldValueString(
	MessageDecomposer *decomposer,
	void *env,
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor *field,
	void* value,
	int size,
	int index
);

/**
 * @brief return field names
 * @return message type
 */
std::string getFieldNames(
	std::vector <std::string> &retval,
	Pkt2OptionsCache *options,
	const std::string &messageTypeName
);

int create_sql(
	std::ostream *output,
	Pkt2OptionsCache *options,
	Packet2Message *packet2Message,
	const std::string &messageType,
	SQL_DIALECT sqldialect,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
);

int create_sql2(
	std::ostream *output,
	Pkt2OptionsCache *options,
	Packet2Message *packet2Message,
	const std::string &messageType,
	SQL_DIALECT sqldialect,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
);

#endif
