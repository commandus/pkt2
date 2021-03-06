#include "str-pkt2.h"

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#endif

#include <sstream>
#include <iomanip>

#include "platform.h"
#include "pbjson.hpp"

#include "utilstring.h"
#include "utilprotobuf.h"
#include "messageformat.h"

#include "env-pkt2.h"
#include "packet-protobuf-message.h"

/**
 * Initialize packet declarations
 * @param proto_path path to the catalog with protobuf decraration files
 * @param verbosity if 1, 2 or 3 print out to the stderr errors parsing declarations
 * @return structure of the packet declaratuions to be passed to the parsePacket()
 */
void* initPkt2(
	const std::string &proto_path,
	int verbosity
)
{
	return (void *) new EnvPkt2(proto_path, verbosity);
}

/**
 * Destroy and free packet declarations
 * @param env packet declaratuions
 */
void donePkt2(void *env)
{
	if (env)
		delete (EnvPkt2*) env;
}

/**
 * Return field name list string
 * @param env contains options_cache
 * @param message_type
 * @param delimiter "\t" or ", "
 * @return field name list
 */
std::string headerFields(
	void *env, 
	const std::string &message_type,
	const std::string &delimiter
)
{
	EnvPkt2* e = (EnvPkt2*) env; 
	std::stringstream ss;
	std::vector <std::string> fldnames;
	std::string t = getFieldNames(fldnames, e->options_cache, message_type);
	ss << t;
	for (std::vector <std::string>::const_iterator it = fldnames.begin(); it != fldnames.end(); it++) {
		ss << delimiter << *it;
	}
	return ss.str();
}

/**
 * Parse packet by declaration
 * @param env packet declaratuions
 * @param inputFormat 0- binary, 1- hex string
 * @param outputFormat 0- json(default), 1- csv, 2- tab, 3- sql, 4- Sql, 5- pbtext, 6- dbg, 7- hex, 8- bin, 11- csv header, 12- tab header
 * @param packet data
 * @param forceMessage "" If specifed, try only message type
 * @return empty string if fails
 */
std::string parsePacket(
	void *env, 
	int inputFormat,
	int outputFormat,
	int sqlDialect,
	const std::string &packet,
	const std::string &forceMessage,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
)
{
	google::protobuf::Message *m;
	if (!parsePacket2Message(&m, env, inputFormat, packet, forceMessage, tableAliases, fieldAliases, properties))
		return "";

	EnvPkt2* e = (EnvPkt2*) env; 
	
	MessageTypeNAddress messageTypeNAddress(m->GetTypeName());
	std::stringstream ss;
	switch (outputFormat)
	{
	case MODE_JSON:
		put_json(&ss, e->options_cache, &messageTypeNAddress, m, tableAliases, fieldAliases, properties);
		break;
	case MODE_CSV:
		put_csv(&ss, e->options_cache, &messageTypeNAddress, m,
			tableAliases, fieldAliases, properties);
		break;
	case MODE_TAB:
		put_tab(&ss, e->options_cache, &messageTypeNAddress, m,
			tableAliases, fieldAliases, properties);
		break;
	case MODE_SQL:
		put_sql(&ss, e->options_cache, &messageTypeNAddress, m, tableAliases, fieldAliases, properties, sqlDialect);
		break;
	case MODE_SQL2:
		put_sql2(&ss, e->options_cache, &messageTypeNAddress, m, tableAliases, fieldAliases, properties, sqlDialect);
		break;
	case MODE_PB_TEXT:
		put_protobuf_text(&ss, e->options_cache, &messageTypeNAddress, m);
		break;
	case MODE_PRINT_DBG:
		put_debug(&ss, &messageTypeNAddress, m);
		break;
	case 7:	
		{
		std::string s(stringDelimitedMessage(&messageTypeNAddress, *m));
		ss << pkt2utilstring::hexString(s.c_str(), s.size());
		}
		break;			
	case 8:	
		ss << stringDelimitedMessage(&messageTypeNAddress, *m);
		break;			
	case 11:	
		ss << headerFields(e, messageTypeNAddress.message_type, ", ")
			<< std::endl;
		put_csv(&ss, e->options_cache, &messageTypeNAddress, m,
			tableAliases, fieldAliases, properties);
		break;			
	case 12:	
		ss << headerFields(e, messageTypeNAddress.message_type, "\t")
			<< std::endl;
		put_tab(&ss, e->options_cache, &messageTypeNAddress, m,
			tableAliases, fieldAliases, properties);
		break;			
	}
	if (m) 
		delete m;
	return ss.str();
}

/**
 * Parse packet by declaration
 * @param retMessage return message of google::protobuf::Message type
 * @param env packet declaratuions
 * @param inputFormat 0- binary, 1- hex string
 * @param packet data
 * @param forceMessage "" If specifed, try only message type
 * @param tableAliases protobuf message to datanase table map
 * @param fieldAliases protobuf message attribute to datanase column map
 * @param properties "session environment variables", e.g addr, eui, time, timestamp
 * @return empty string if fails
 */
bool parsePacket2ProtobufMessage(
	void **retMessage,
	void *env, 
	int inputFormat,
	const std::string &packet,
	const std::string &forceMessage,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
) {
	return parsePacket2Message((google::protobuf::Message **) retMessage, env, inputFormat, packet, 
		forceMessage, tableAliases, fieldAliases, properties);
}

/**
 * Return CREATE table SQL clause
 * @param env packet declaratuions
 * @param messageName Protobuf full type name (including packet)
 * @param outputFormat 3- sql, 4- Sql
 * @param sqlDialect 0- PostgeSQL, 1- MySQL, 1- Firebird
 * @param tableAliases <Protobuf full type name>=<alias (SQL table name)>
 * @param fieldAliases <Protobuf message fiekd name>=<alias (SQL column name)>
 * @return empty string if fails
 */
std::string createTableSQLClause(
	void *env, 
	const std::string &messageName,
	int outputFormat,
	int sqlDialect,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
) {
	EnvPkt2* e = (EnvPkt2*) env; 
	
	std::stringstream ss;
	switch (outputFormat)
	{
	case MODE_SQL:
		create_sql(&ss, e->options_cache, e->packet2Message, messageName, (SQL_DIALECT) sqlDialect, tableAliases, fieldAliases, properties);
		break;
	case MODE_SQL2:
		create_sql2(&ss, e->options_cache, e->packet2Message, messageName, (SQL_DIALECT) sqlDialect, tableAliases, fieldAliases, properties);
		break;
	}
	return ss.str();
}
