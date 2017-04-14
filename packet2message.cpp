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

#include "pkt2.pb.h"
#include "packet2message.h"
#include "pkt2packetvariable.h"
#include "messagecomposer.h"
#include "platform.h"
#include "errorcodes.h"
#include "utilstring.h"
#include "utilprotobuf.h"
#include "javascript-context.h"

PacketParseEnvironment::PacketParseEnvironment
(
	    struct sockaddr *socket_address_src,
	    struct sockaddr *socket_address_dst,
		const std::string &apacket,
		Pkt2OptionsCache *options_cache,
		const std::string &force_message
)
	: context(NULL), packet(apacket)
{
	bool found;
	pv = &(!force_message.empty()
			? options_cache->getPacketVariable(force_message, &found)
					: options_cache->find1(packet, &found));
	if (!found)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << force_message;
	}
	context = getJavascriptContext(pv, socket_address_src, socket_address_dst, packet);
}

PacketParseEnvironment::~PacketParseEnvironment()
{
	if (context != NULL)
	{
		duk_pop(context);
		duk_destroy_heap(context);
	}
}

Packet2Message::Packet2Message
(
	const ProtobufDeclarations *protobufdeclarations,
	const Pkt2OptionsCache *optionscache,
	int verb
)
	: declarations(protobufdeclarations), options_cache(optionscache), verbosity(verb)
{
}

Packet2Message::~Packet2Message()
{
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

/**
 * MessageComposer callback
 * @param env
 * @param message_descriptor
 * @param field_type
 * @param field_number
 * @param repeated
 * @param index
 * @param retval
 * @return
 */
bool oncomposefield (
	void* env,
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor::CppType field_type,
	int field_number,
	bool repeated,
	int index,
	std::string &retval
)
{
	PacketParseEnvironment *e = (PacketParseEnvironment *) env;
	const FieldNameVariable *v = e->pv->getVariableByFieldNumber(field_number);
	duk_eval_string(e->context, v->var.get().c_str());
	retval = duk_safe_to_string(e->context, -1);
	// LOG(INFO) << message_descriptor->full_name() << "." << v->field_name << "=" << retval;
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
 * @param PacketParseEnvironment *env
 * @return
 */
google::protobuf::Message *Packet2Message::parsePacket
(
	PacketParseEnvironment *env
)
{
	// Create Javascript context with global object field.xxx
	google::protobuf::Message *m = declarations->getMessage(env->pv->message_type);
	if (m)
	{
		MessageComposer mc(env, options_cache, m, oncomposefield, onnextmessage);

		if (verbosity >= 2)
		{
			// packet input fields
			for (int i = 0; i < env->pv->packet.fields_size(); i++)
			{
				pkt2::Field f = env->pv->packet.fields(i);
				LOG(INFO) << f.name() << ": " << hexString(extractField(env->packet, f));
			}
			// packet output variables
			for (int i = 0; i < env->pv->fieldname_variables.size(); i++)
			{
				FieldNameVariable v = env->pv->fieldname_variables[i];
				LOG(INFO) << m->GetTypeName() << "." << v.field_name << " = "
						<< extractVariable(env->context, v) << " " << v.var.measure_unit() << " (" << v.var.full_name() << ")";
			}
		}
	}

	return m;
}
