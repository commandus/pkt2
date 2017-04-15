/*
 * javascript-context.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: andrei
 */

#include <arpa/inet.h>
#include "javascript-context.h"
#include "utilprotobuf.h"
#include "pkt2.pb.h"

void duk_fatal_handler(void *udata, const char *msg)
{
	fprintf(stderr, "Javascript error: %s\n", (msg ? msg : ""));
	fflush(stderr);
	abort();
}

/**
 * Create Javascript context with global object field.xxx
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
)
{
	duk_context *ctx = duk_create_heap(NULL, NULL, NULL, (void *) &packet, duk_fatal_handler);

	// current time
	time_t t =  time(NULL);
	duk_push_global_object(ctx);
	duk_push_uint(ctx, t);
	duk_put_prop_string(ctx, -2, "time");

	// src.ip, src.port
	putSocketAddress(ctx, "src", socket_src);
	// dst
	putSocketAddress(ctx, "dst", socket_dst);

	// field
	duk_push_global_object(ctx);
	duk_push_object(ctx);

	for (int i = 0; i < pv->packet.fields_size(); i++)
	{
		const pkt2::Field &f = pv->packet.fields(i);
		pushField(ctx, packet, f);
		duk_put_prop_string(ctx, -2, f.name().c_str());
	}
	duk_put_global_string(ctx, "field");
	return ctx;
}

duk_context *getFormatJavascriptContext
(
		void *env
)
{
	duk_context *ctx = duk_create_heap(NULL, NULL, NULL, env, duk_fatal_handler);

	// current time
	time_t t =  time(NULL);
	duk_push_global_object(ctx);
	duk_push_uint(ctx, t);
	duk_put_prop_string(ctx, -2, "time");

	return ctx;
}

void putSocketAddress
(
	duk_context * ctx,
	const std::string &objectName,
	struct sockaddr *socket_addr
)
{
	duk_push_global_object(ctx);
	duk_push_object(ctx);
	std::string ip = "";
	uint32_t ip4 = 0;
	uint32_t port = 0;
	char str[INET6_ADDRSTRLEN];

	if (socket_addr)
	{
		if (socket_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *in = (struct sockaddr_in*) socket_addr;
			ip4 = in->sin_addr.s_addr;
			port = in->sin_port;
			inet_ntop(AF_INET, &(in->sin_addr.s_addr), str, INET_ADDRSTRLEN);
			ip = std::string(str);
		}
		if (socket_addr->sa_family == AF_INET6)
		{
			struct sockaddr_in6 *in = (struct sockaddr_in6*) socket_addr;
			port = in->sin6_port;
			inet_ntop(AF_INET6, &(in->sin6_addr), str, INET6_ADDRSTRLEN);
			ip = std::string(str);

		}
	}

	duk_push_uint(ctx, ip4);
	duk_put_prop_string(ctx, -2, "ip4");

	duk_push_uint(ctx, port);
	duk_put_prop_string(ctx, -2, "port");

	duk_push_string(ctx, ip.c_str());
	duk_put_prop_string(ctx, -2, "ip");

	duk_put_global_string(ctx, objectName.c_str());
}

void pushField
(
		duk_context *ctx,
		const std::string &packet,
		const pkt2::Field &field
)
{
	switch (field.type()) {
		case pkt2::INPUT_NONE:
			duk_push_false(ctx);
			break;
		case pkt2::INPUT_CHAR:
		case pkt2::INPUT_STRING:
			duk_push_string(ctx, packet.substr(field.offset(), field.size()).c_str());
			break;
		case pkt2::INPUT_DOUBLE:
			duk_push_number(ctx, extractFieldDouble(packet, field));
			break;
		case pkt2::INPUT_BYTES:
		{
			duk_idx_t arr_idx = duk_push_array(ctx);
			int c = 0;
			for (int i = field.offset(); i < field.size(); i++)
			{
				duk_push_int(ctx, packet[i]);
				duk_put_prop_index(ctx, arr_idx, c);
				c++;
			}
			break;
		}
		default:
			duk_push_uint(ctx, extractFieldUInt(packet, field));
			break;
	}

}
