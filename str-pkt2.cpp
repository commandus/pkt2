#include "str-pkt2.h"

#include <netinet/in.h>
#include <sstream>
#include <iomanip>

#include "platform.h"
#include "pbjson.hpp"

#include "utilstring.h"
#include "utilprotobuf.h"
#include "messageformat.h"

using namespace google::protobuf;
using namespace google::protobuf::io;

EnvPkt2::EnvPkt2(
	const std::string &proto_path,
	int verbosity
) {
	declarations = new ProtobufDeclarations(proto_path, verbosity);
	options_cache = new Pkt2OptionsCache(declarations);
	packet2Message = new Packet2Message(declarations, options_cache, verbosity);
}

EnvPkt2::~EnvPkt2() {
	if (packet2Message) {
		delete packet2Message;
	}
	if (options_cache) {
		delete options_cache;
	}
	if (declarations) {
		delete declarations;
	}
}

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
	const std::string &packet,
	const std::string &forceMessage
)
{
	google::protobuf::Message *m;
	if (!parsePacket2Message(&m, env, inputFormat, packet, forceMessage))
		return "";

	EnvPkt2* e = (EnvPkt2*) env; 
	
	MessageTypeNAddress messageTypeNAddress(m->GetTypeName());
	std::stringstream ss;
	switch (outputFormat)
	{
	case MODE_JSON:
		put_json(&ss, e->options_cache, &messageTypeNAddress, m);
		break;
	case MODE_CSV:
		put_csv(&ss, e->options_cache, &messageTypeNAddress, m);
		break;
	case MODE_TAB:
		put_tab(&ss, e->options_cache, &messageTypeNAddress, m);
		break;
	case MODE_SQL:
		put_sql(&ss, e->options_cache, &messageTypeNAddress, m);
		break;
	case MODE_SQL2:
		put_sql2(&ss, e->options_cache, &messageTypeNAddress, m);
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
		put_csv(&ss, e->options_cache, &messageTypeNAddress, m);
		break;			
	case 12:	
		ss << headerFields(e, messageTypeNAddress.message_type, "\t")
			<< std::endl;
		put_tab(&ss, e->options_cache, &messageTypeNAddress, m);
		break;			
	}
	if (m) 
		delete m;
	return ss.str();
}

/**
 * Parse packet by declaration
 * @param retMessage return message
 * @param env packet declaratuions
 * @param inputFormat 0- binary, 1- hex string
 * @param packet data
 * @param forceMessage "" If specifed, try only message type
 * @return empty string if fails
 */
bool parsePacket2Message(
	google::protobuf::Message **retMessage,
	void *env, 
	int inputFormat,
	const std::string &packet,
	const std::string &forceMessage
)
{
	struct sockaddr_in s, d;
	memset(&s, 0, sizeof(sockaddr_in));
	memset(&d, 0, sizeof(sockaddr_in));

	EnvPkt2* e = (EnvPkt2*) env; 
	const std::string *data;
	std::string bdata;
	switch (inputFormat) {
	case 1:	// hex
		if (packet.length()) {
			bdata = pkt2utilstring::hex2string(packet);
			data = &bdata;
		}
		break;
	default:	// binary
		data = &packet;
		break;
	}
	
	PacketParseEnvironment packet_env((struct sockaddr *) &s, (struct  sockaddr *) &d, *data, 
		e->options_cache, forceMessage);
	*retMessage = e->packet2Message->parsePacket(&packet_env);
	return *retMessage != NULL;
}
