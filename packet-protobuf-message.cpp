
#include "packet-protobuf-message.h"

#include "env-pkt2.h"
#include "utilstring.h"

/**
 * Parse packet by declaration
 * @param retMessage return message
 * @param env packet declaratuions
 * @param inputFormat 0- binary, 1- hex string
 * @param packet data
 * @param forceMessage "" If specifed, try only message type
 * @param tableAliases protobuf message to datanase table map
 * @param fieldAliases protobuf message attribute to datanase column map
 * @param properties "session environment variables", e.g addr, eui, time, timestamp
 * @return empty string if fails
 */
bool parsePacket2Message(
	google::protobuf::Message **retMessage,
	void *env, 
	int inputFormat,
	const std::string &packet,
	const std::string &forceMessage,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases,
	const std::map<std::string, std::string> *properties
)
{
	struct sockaddr_in s, d;
	memset(&s, 0, sizeof(sockaddr_in));
	memset(&d, 0, sizeof(sockaddr_in));
	
	const std::string *data;
	std::string bdata;
	if (inputFormat == 1 && packet.length()) {	// hex
		bdata = pkt2utilstring::hex2string(packet);
		data = &bdata;
	} else {
		data = &packet;
	}
	
	PacketParseEnvironment packet_env((struct sockaddr *) &s, (struct  sockaddr *) &d, *data, 
		((EnvPkt2*) env)->options_cache, forceMessage, tableAliases, fieldAliases, properties);
	*retMessage = ((EnvPkt2*) env)->packet2Message->parsePacket(&packet_env);
	return *retMessage != NULL;
}

/**
 * Get message by type name
 * @param retMessage return message (NULL if fail)
 * @param env packet declaratuions
 * @param messageType,
 * @return false if fails
 */
bool getMessageByName(
	google::protobuf::Message **retMessage,
	void *env, 
	const std::string &messageType
)
{
	*retMessage = ((EnvPkt2*) env)->packet2Message->getMessageByName(messageType);
	return *retMessage != NULL;
}
