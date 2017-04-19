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

/**
 * @brief Create Javascript context with global object field.xxx for parsing
 * @param pv root packet and variables
 * @param socket_src source IP address
 * @param socket_dst destination IP address
 * @param packet data
 * @return Javascipt context containing "value", "time" objects
 */
duk_context *getJavascriptContext
(
	Pkt2OptionsCache *optionscache,
    const Pkt2PacketVariable *packet_root_variable,
	struct sockaddr *socket_src,
	struct sockaddr *socket_dst,
	const std::string &packet
);

duk_context *getFormatJavascriptContext
(
		void *ctx
);

void duk_fatal_handler(void *udata, const char *msg);

#endif /* JAVASCRIPT_CONTEXT_H_ */
