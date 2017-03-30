/*
 * packet2message.cpp
 *
 * time
 * src.ip
 *
 *
 *
	// src.ip, src.port
	putSocketAddress(ctx, "src", socket_src);
	// dst
	putSocketAddress(ctx, "dst", socket_dst);

	// field
	duk_push_global_object(ctx);
	duk_push_object(ctx);

	for (int i = 0; i < pv.packet.fields_size(); i++)
	{
		const pkt2::Field &f = pv.packet.fields(i);
		pushField(ctx, packet, f);
		duk_put_prop_string(ctx, -2, f.name().c_str());
	}
	duk_put_global_string(ctx, "field");

 */

#include <string.h>
#include <algorithm>

#include <glog/logging.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "duk/duktape.h"

#include "packet2message.h"
#include "pkt2packetvariable.h"
#include "messagecomposer.h"
#include "platform.h"
#include "errorcodes.h"
#include "utilstring.h"

Packet2Message::Packet2Message(
		const std::string &protopath,
		int verbose
)
	: verbosity(verbose)
{
	declarations = new ProtobufDeclarations(protopath, verbosity);
	options_cache = new Pkt2OptionsCache(declarations);
}

Packet2Message::~Packet2Message() {
	delete options_cache;
	delete declarations;
}

/**
 * Get field value from the packet
 * @param packet
 * @param field
 * @return
 */
std::string extractField
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	int sz = packet.size();
	if (sz < field.offset() + field.size())
	{
		LOG(ERROR) << ERR_PACKET_TOO_SMALL << field.name() << " " << field.offset() << " " << field.size() << " " << sz;
		return "";
	}
	std::string r = packet.substr(field.offset(), field.size());
	if (ENDIAN_NEED_SWAP(field.endian()))
	{
		char *p = &r[0];
		std::reverse(p, p + field.size());
	}
	return r;
}

/**
 * Get field value from the packet as 32 bit integer
 * @param packet
 * @param field
 * @return
 */
uint32_t extractFieldUInt
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 0:
		return 0;
	case 1:
		return *((uint8_t*) &r[0]);
	case 2:
		return *((uint16_t*) &r[0]);
	case 4:
		return *((uint32_t*) &r[0]);
	default:
	{
		uint64_t v = 0;
		memmove(&v, &r[0], (sz < sizeof(uint64_t) ? sz : sizeof(uint64_t)));
		return v;
	}
	}
	return 0;
}

/**
 * Get field value from the packet as double
 * @param packet
 * @param field
 * @return
 */
double extractFieldDouble
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 4:
		return *((float*) &r[0]);
	case 8:
		return *((double*) &r[0]);
	}
	return 0;
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

/**
 * Create Javascript context with global object field.xxx
 * @param pv
 * @param socket_address_src
 * @param socket_address_dst
 * @param packet
 * @return
 */
duk_context *makeJavascriptContext
(
	const Pkt2PacketVariable &pv,
	struct sockaddr *socket_src,
	struct sockaddr *socket_dst,
	const std::string &packet
)
{
	duk_context *ctx = duk_create_heap_default();

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

	for (int i = 0; i < pv.packet.fields_size(); i++)
	{
		const pkt2::Field &f = pv.packet.fields(i);
		pushField(ctx, packet, f);
		duk_put_prop_string(ctx, -2, f.name().c_str());
	}
	duk_put_global_string(ctx, "field");
	return ctx;
}

/**
 * Get variable from the packet as string
 * @param ctx
 * @param variable
 * @return
 */
std::string extractVariable
(
	duk_context *ctx,
	const FieldNameVariable &variable
)
{
	duk_eval_string(ctx, variable.var.get().c_str());
    return duk_safe_to_string(ctx, -1);
}

bool oncomposefield (
	void* env,
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor::CppType field_type,
	const std::string &message_name,
	const std::string &field_name,
	bool repeated,
	int index,
	std::string &retval
)
{
	LOG(INFO) << message_name << "." << field_name;
	retval = "0";
	return false;
}

bool onnextmessage
(
	void* env,
	google::protobuf::Message* message,
	int index
)
{
	return false;
}

/**
 * Parse packet
 * @param socket_address_src can be NULL
 * @param socket_address_dst can be NULL
 * @param packet
 * @param force_message packet.message or "" (no force)
 * @return
 */
google::protobuf::Message *Packet2Message::parse
(
    struct sockaddr *socket_address_src,
    struct sockaddr *socket_address_dst,
	const std::string &packet,
	const std::string &force_message
)
{
	bool found;
	const Pkt2PacketVariable &pv = (!force_message.empty()
			? options_cache->getPacketVariable(force_message, &found)
					: options_cache->getPacketVariable("example1.TemperaturePkt", &found));

	if (!found)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << force_message;
		return NULL;
	}

	// Create Javascript context with global object field.xxx
	duk_context *jsCtx = makeJavascriptContext(pv, socket_address_src, socket_address_dst, packet);
	if (verbosity >= 2)
	{
		// packet input fields
		for (int i = 0; i < pv.packet.fields_size(); i++)
		{
			pkt2::Field f = pv.packet.fields(i);
			LOG(INFO) << f.name() << ": " << hexString(extractField(packet, f));
		}
		// packet output variables
		for (int i = 0; i < pv.fieldname_variables.size(); i++)
		{
			FieldNameVariable v = pv.fieldname_variables[i];
			LOG(INFO) << v.field_name << "(" << v.var.full_name() << ") = " << extractVariable(jsCtx, v) << v.var.measure_unit();
		}
	}

	google::protobuf::Message *m = declarations->getMessage(pv.message_type);

	if (m)
	{
		// TODO
		MessageComposer mc(this, options_cache, m, oncomposefield, onnextmessage);
	}

	duk_pop(jsCtx);
	duk_destroy_heap(jsCtx);

	return m;
}
