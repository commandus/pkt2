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

class PacketParseEnvironment
{
public:
	const Pkt2PacketVariable *pv;
	duk_context *context;
	PacketParseEnvironment
	(
			const Pkt2PacketVariable *pvar,
			duk_context *ctx
	);
};


class Packet2Message {
private:
	int verbosity;
	ProtobufDeclarations *declarations;
	Pkt2OptionsCache *options_cache;
public:
	Packet2Message
	(
		const std::string &protopath,
		int verbosity
	);
	virtual ~Packet2Message();

	google::protobuf::Message *parse
	(
	    struct sockaddr *socket_address_src,
	    struct sockaddr *socket_address_dst,
		const std::string &packet,
		const std::string &force_message
	);
	/**
	 * Parse packet
	 * @param socket_address_src can be NULL
	 * @param socket_address_dst can be NULL
	 * @param packet
	 * @param size
	 * @param force_message packet.message or "" (no force)
	 * @return
	 */
	google::protobuf::Message *parse
	(
	    struct sockaddr *socket_address_src,
	    struct sockaddr *socket_address_dst,
		void *packet,
		int size,
		const std::string &force_message
	);

};

#endif /* PACKET2MESSAGE_H_ */
