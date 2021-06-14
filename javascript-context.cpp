/**
 * @file javascript-context.cpp
 */
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include "javascript-context.h"
#include "utilprotobuf.h"
#include "utilstring.h"
#include "pkt2.pb.h"

#define	DUK_DEBUG(context) \
	duk_push_context_dump(context); \
	std::cout << duk_to_string(context, -1) << std::endl; \
	duk_pop(context); \

void putSocketAddress
(
	duk_context * ctx,
	const std::string &objectName,
	struct sockaddr *socket_addr
);

void pushMessage
(
	duk_context *ctx,
	Pkt2OptionsCache *options_cache,
	const Pkt2PacketVariable *packet_variable,
	const std::string &packet
);

/**
  * @brief For formatting
  */
JavascriptContext::JavascriptContext()
	: expression(NULL)
{
	context = duk_create_heap(NULL, NULL, NULL, this, duk_fatal_handler_javascript_context);
	// current time
	time_t t =  time(NULL);
	duk_push_global_object(context);
	duk_push_uint(context, t);
	duk_put_prop_string(context, -2, "time");
}

JavascriptContext::~JavascriptContext()
{
	duk_pop(context);
	duk_destroy_heap(context);
}

/**
 * @brief Create Javascript context with global object field.xxx
 * @param options_cache option cache
 * @param packet_root_variable "root" packet input fields and output variables
 * @param socket_src IP v4 source address
 * @param socket_dst IP v4 destination address
 * @param packet data
 * @see pushField
 */
 JavascriptContext::JavascriptContext
(
	Pkt2OptionsCache *options_cache,
	const Pkt2PacketVariable *packet_root_variable,
	struct sockaddr *socket_src,
	struct sockaddr *socket_dst,
	const std::string &packet
)
{
	context = duk_create_heap(NULL, NULL, NULL, this, duk_fatal_handler_javascript_context);

	// current time
	time_t t =  time(NULL);
	
	duk_push_global_object(context);
	duk_push_uint(context, t);
	duk_put_prop_string(context, -2, "time");
	// src.ip, src.port
	putSocketAddress(context, "src", socket_src);
	// dst
	putSocketAddress(context, "dst", socket_dst);
	// field
	duk_push_global_object(context);
    pushMessage(context, options_cache, packet_root_variable, packet);
	duk_put_prop_string(context, -2, "field");
	duk_pop(context);
}

/**
 * @brief Create Javascript context with global object field.xxx
 * @param options_cache option cache
 * @param packet_root_variable "root" packet input fields and output variables
 * @param packet data
 * @see pushField
 */
 JavascriptContext::JavascriptContext
(
	Pkt2OptionsCache *options_cache,
	const Pkt2PacketVariable *packet_root_variable,
	const std::string &packet
)
{
	context = duk_create_heap(NULL, NULL, NULL, this, duk_fatal_handler_javascript_context);

	// current time
	time_t t =  time(NULL);
	duk_push_global_object(context);
	duk_push_uint(context, t);
	duk_put_prop_string(context, -2, "time");

	// field
	duk_push_global_object(context);
    pushMessage(context, options_cache, packet_root_variable, packet);
    duk_put_prop_string(context, -2, "field");
	duk_pop(context);
}

/**
 * @brief Javascript error handler
 * @param env JavascriptContext object
 * @param msg error message
 * 
 */
void duk_fatal_handler_javascript_context(
	void *env, 
	const char *msg
)
{
	if (env && ((JavascriptContext *) env)->expression)
	{
		JavascriptContext *jenv = (JavascriptContext *) env;
		fprintf(stderr, "Javascript error: %s in \n%s\n", (msg ? msg : ""), jenv->expression->c_str());
	}
	else
		fprintf(stderr, "Javascript error: %s\n", (msg ? msg : ""));
	fflush(stderr);
	abort();
}

/**
 * @brief Add field value to the Javascript "field" global object
 * @param ctx Jvascript context
 * @param packet data
 * @param field input data field
 * @see getJavascriptContext()
 */
void pushField
(
	duk_context *ctx,
	const std::string &packet,
	const pkt2::Field &field
)
{
	switch (field.type()) {
		case pkt2::INPUT_MESSAGE:
			break;
		case pkt2::INPUT_NONE:
			duk_push_false(ctx);
			duk_put_prop_string(ctx, -2, field.name().c_str());
			break;
		case pkt2::INPUT_CHAR:
		case pkt2::INPUT_STRING:
			duk_push_string(ctx, packet.substr(field.offset(), field.size()).c_str());
			duk_put_prop_string(ctx, -2, field.name().c_str());
			break;
		case pkt2::INPUT_DOUBLE:
			duk_push_number(ctx, extractFieldDouble(packet, field));
			duk_put_prop_string(ctx, -2, field.name().c_str());
			break;
		case pkt2::INPUT_BYTES:
		{
			duk_idx_t arr_idx = duk_push_array(ctx);
			int c = 0;
			int sz = field.offset() + field.size();
			for (int i = field.offset(); i < sz; i++)
			{
				duk_push_int(ctx, packet[i]);
				duk_put_prop_index(ctx, arr_idx, c);
				c++;
			}
			duk_put_prop_string(ctx, -2, field.name().c_str());
			break;
		}
		case pkt2::INPUT_INT:
			{
				duk_push_uint(ctx, extractFieldInt(packet, field));
				duk_put_prop_string(ctx, -2, field.name().c_str());
			}
			break;
		default:
			duk_push_uint(ctx, extractFieldUInt(packet, field));
			duk_put_prop_string(ctx, -2, field.name().c_str());
			break;
	}
}

/**
 * @brief Put socket address object with properties:
 * - ip		IP v.4 address, string
 * - ip4	IP address, 32 bit integer
 * - port	IP port number, 16 bit integer
 * @param ctx Javascript context
 * @param objectName name of Javascript object
 * @param socket_addr IP v4 address
 */
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
	duk_put_prop_string(ctx, -2, objectName.c_str());
//	duk_put_global_string(ctx, objectName.c_str());
	duk_pop(ctx);
}

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
)
{
	// get a "zone" inside a packet corresponding to the message in the entire data
	const FieldNameVariable *fnv = packet_variable->getVariableByFieldNumber(fd->number());
	if (!fnv) 
	{
		// TODO report error
		return -1;
	}
	// extract "zone" (field in the parent message) by name in the variable's "get" property.
	// get can be: "field.name" (preferred) or "name" (optional)
	std::string zone = fnv->var.get();
	// if "get" contains "field." prefix, remove it
	std::string::size_type pos = zone.find("field.");
	if (pos != std::string::npos)
	{
		zone = zone.substr(pos + 6);
	}

	// Search field in the parent packet fields by the name
	int z = -1;
	for (int j = 0; j < packet_variable->packet.fields_size(); j++)
	{
		if (packet_variable->packet.fields(j).name() == zone)
		{
			z = j;
			break;
		}
	}
	return z;
}

/**
 * @brief Add field value to the Javascript "field" global object
 * @param ctx Jvascript context
 * @param packet data
 * @param field input data field
 * @see getJavascriptContext()
 */
void pushMessage
(
    duk_context *ctx,
	Pkt2OptionsCache *options_cache,
    const Pkt2PacketVariable *packet_variable,
	const std::string &packet
)
{
	duk_push_object(ctx);

	// put non-message fields
    for (int i = 0; i < packet_variable->packet.fields_size(); i++)
    {
        const pkt2::Field &f = packet_variable->packet.fields(i);
        pushField(ctx, packet, f);
    }

    // iterate embedded messages and call pushMessage recursively
	const google::protobuf::Descriptor* md = options_cache->protobuf_decrarations->getMessageDescriptor(packet_variable->message_type);
	for (int f = 0; f < md->field_count(); f++)
	{
		const google::protobuf::FieldDescriptor* fd = md->field(f);
		// we need field with embedded message only
		if (fd->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
			continue;
		
		// get packet for embedded message, find out in the options_cache
		bool found;
		// iridium.IE_Packet.ioheader
		const Pkt2PacketVariable &packet_var = options_cache->getPacketVariable(fd->message_type()->full_name(), &found);
		if (!found) 
		{
			// TODO report error
			continue;
		}
		
		int z = findFieldNumber(packet_variable, fd);
		if (z < 0)
			continue;
			
		// extract field from the parent packet
		const pkt2::Field &fld = packet_variable->packet.fields(z);
		std::string field_packet;
		if (fld.offset() < packet.size())
			field_packet = packet.substr(fld.offset(), fld.size());
		else
			field_packet = "";
		pushMessage(ctx, options_cache, &packet_var, field_packet);
		duk_put_prop_string(ctx, -2, fd->name().c_str());
	}
}
