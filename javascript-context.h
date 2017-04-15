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

/**
 * Create Javascript context with global object field.xxx for parsing
 * @param pv
 * @param socket_address_src
 * @param socket_address_dst
 * @param packet
 * @return
 */
duk_context *getJavascriptContext
(
	const Pkt2PacketVariable *pv,
	struct sockaddr *socket_src,
	struct sockaddr *socket_dst,
	const std::string &packet
);

duk_context *getFormatJavascriptContext
(
);

void putSocketAddress
(
	duk_context * ctx,
	const std::string &objectName,
	struct sockaddr *socket_addr
);

void pushField
(
		duk_context *ctx,
		const std::string &packet,
		const pkt2::Field &field
);

#endif /* JAVASCRIPT_CONTEXT_H_ */
