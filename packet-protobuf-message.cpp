
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
