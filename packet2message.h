/*
 * packet2message.h
 *
 */

#ifndef PACKET2MESSAGE_H_
#define PACKET2MESSAGE_H_

#include <google/protobuf/message.h>

#include "duk/duktape.h"

#include "pkt2optionscache.h"
#include "protobuf-declarations.h"

/**
 * @brief Packet container with initialize javascript environment ready to parse.
 * The main reason why packet + javascript environment are separated is
 * re-use by different parsers.
 */
class PacketParseEnvironment
{
public:
	const std::string &packet;
	const Pkt2PacketVariable *pv;
	duk_context *context;
	PacketParseEnvironment
	(
		struct sockaddr *socket_address_src,
		struct sockaddr *socket_address_dst,
		const std::string &packet,
		Pkt2OptionsCache *options_cache,
		const std::string &force_message
	);
	~PacketParseEnvironment();
};


class Packet2Message {
private:
	int verbosity;
	const ProtobufDeclarations *declarations;
	const Pkt2OptionsCache *options_cache;
public:
	Packet2Message
	(
		const ProtobufDeclarations *protobufdeclarations,
		const Pkt2OptionsCache *optionscache,
		int verbosity
	);
	virtual ~Packet2Message();

	/**
	 * Parse packet
	 * @param socket_address_src can be NULL
	 * @param socket_address_dst can be NULL
	 * @param packet
	 * @param size
	 * @param force_message packet.message or "" (no force)
	 * @return
	 */
	google::protobuf::Message *parsePacket
	(
		PacketParseEnvironment *env
	);

};

#endif /* PACKET2MESSAGE_H_ */
