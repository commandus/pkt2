/*
 * javascript-context.h
 *
 *  Created on: Apr 14, 2017
 *      Author: andrei
 */

#ifndef JAVASCRIPT_CONTEXT_H_
#define JAVASCRIPT_CONTEXT_H_

#include "duk/duktape.h"
#include "pkt2.pb.h"
#include "pkt2packetvariable.h"
#include "pkt2optionscache.h"

class JavascriptContext
{
public:
	/**
	 * For formatting
	 */
	JavascriptContext();
	~JavascriptContext();

	/**
	 * For parsing packet
	 */
	JavascriptContext(
		Pkt2OptionsCache *optionscache,
		const Pkt2PacketVariable *packet_root_variable,
		const std::string &packet
	);

	/**
	 * For parsing packet
	 */
	JavascriptContext(
		Pkt2OptionsCache *optionscache,
		const Pkt2PacketVariable *packet_root_variable,
		struct sockaddr *socket_src,
		struct sockaddr *socket_dst,
		const std::string &packet
	);

	std::string *expression;
	duk_context *context;
};

void duk_fatal_handler_javascript_context(void *udata, const char *msg);

/**
 * @brief find field number. 
 * Usage: const std::string &field_packet = packet.substr(packet_variable->packet.fields(z).offset(), packet_variable->packet.fields(z).size());
 * @param packet_variable packet variable where to find out
 * @param fd field descriptor to find out
 * @return -1 if not found
 */
int findFieldNumber
(
    const Pkt2PacketVariable *packet_variable,
	const google::protobuf::FieldDescriptor* fd
);

#endif /* JAVASCRIPT_CONTEXT_H_ */
