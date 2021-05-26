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
#include <iostream>
#include <string.h>
#include <algorithm>
#include <sstream>

#ifdef ENABLE_LOG
#include <glog/logging.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pkt2.pb.h"
#include "packet2message.h"
#include "pkt2packetvariable.h"
#include "messagecomposer.h"
#include "platform.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "javascript-context.h"

PacketParseEnvironment::PacketParseEnvironment
(
	    struct sockaddr *socket_address_src,
	    struct sockaddr *socket_address_dst,
		const std::string &apacket,
		Pkt2OptionsCache *optionscache,
		const std::string &force_message,
		const std::map<std::string, std::string> *atableAliases,
		const std::map<std::string, std::string> *afieldAliases
)
	: packet(apacket), options_cache(optionscache),
		tableAliases(atableAliases), fieldAliases(afieldAliases)
{
    bool found;
	if (force_message.empty()) {
		packet_root_variable = &(optionscache->find1(packet, &found));
	} else {
		packet_root_variable = &(optionscache->getPacketVariable(force_message, &found));
		// if (!found) packet_root_variable = &(optionscache->find1(packet, &found));
	}
	if (found)
        context = new JavascriptContext(optionscache, packet_root_variable, socket_address_src, socket_address_dst, packet);
    else {
		packet_root_variable = NULL;
		context = NULL;
	}
}

PacketParseEnvironment::~PacketParseEnvironment()
{
	if (context)
		delete context;
}

std::string PacketParseEnvironment::getFullFieldName()
{
    std::stringstream ss;
    const char* separator = "";
    for (std::deque<std::string>::const_iterator it = fieldnames.begin(); it != fieldnames.end(); ++it)
    {
        ss << separator << *it;
        separator = ".";
    }
    return ss.str();
}

void PacketParseEnvironment::pushFieldName(const std::string &field_name)
{
    fieldnames.push_back(field_name);
}

void PacketParseEnvironment::popFieldName()
{
    fieldnames.pop_back(); 
}

Packet2Message::Packet2Message
(
	const ProtobufDeclarations *protobufdeclarations,
	const Pkt2OptionsCache *optionscache,
	int verb
)
	: verbosity(verb), declarations(protobufdeclarations), options_cache(optionscache)
{
}

Packet2Message::~Packet2Message()
{
}

/**
 * @brief Get variable from the packet as string
 * @param context Javascript context
 * @param variable field name and variable
 * @return value as string
 */
std::string extractVariable
(
	JavascriptContext *context,
	const FieldNameVariable &variable
)
{
	std::string expr = variable.var.get();
	context->expression = &expr;
	duk_eval_string(context->context, expr.c_str());
    return duk_safe_to_string(context->context, -1);
}

/**
 * @brief MessageComposer callback
 * @param env PacketParseEnvironment instance
 * @param message_descriptor Protobuf message
 * @param field_type C++ type 
 * @param field_number protobuf field number
 * @param repeated true- array
 * @param index array index(valid if repeated is true)
 * @param retval return value
 * @return true- success
 * @see PacketParseEnvironment
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
    bool found;
    const Pkt2PacketVariable &pv = e->options_cache->getPacketVariable(message_descriptor->full_name(), &found);
    if (!found)
    {
#ifdef ENABLE_LOG
        LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << " trying compose message " << message_descriptor->full_name() << " fron the packet data, message not found.";
#endif
        return false;
    }
   
	const FieldNameVariable *v = pv.getVariableByFieldNumber(field_number);
	
	std::string expr = v->var.get();
	// if (!e->context) raise
	e->context->expression = &expr;
#ifdef ENABLE_LOG
	LOG(INFO) << ERR_MESSAGE_TYPE_NOT_FOUND << " trying compose message " << message_descriptor->full_name() << " fron the packet data, message not found.";
#endif
	duk_eval_string(e->context->context, expr.c_str());
	retval = duk_safe_to_string(e->context->context, -1);
	return false;
}

bool onmessagebegin
(
	void* env,
    const google::protobuf::FieldDescriptor *field,
	google::protobuf::Message* message,
    bool repeated,
	int index
)
{
    PacketParseEnvironment *e = (PacketParseEnvironment *) env;
    if (field)
        e->pushFieldName(field->name());
    else
        e->pushFieldName("");
	return false;
}

bool onmessageend
(
	void* env,
	google::protobuf::Message* message,
    bool repeated,
	int index
)
{
    PacketParseEnvironment *e = (PacketParseEnvironment *) env;
    e->popFieldName();
    return false;
}

/**
 * @brief Parse packet. Compose message from the packet
 * @param env environment contains Javascript context etc to share with other parsers
 * @return Protobuf message
 */
google::protobuf::Message *Packet2Message::parsePacket
(
	PacketParseEnvironment *env,
	const std::map<std::string, std::string> *tableAliases,
	const std::map<std::string, std::string> *fieldAliases
)
{
	if (!env->packet_root_variable)
		return NULL;

	// Create Javascript context with global object field.xxx
	google::protobuf::Message *m = declarations->getMessage(env->packet_root_variable->message_type);
	if (m)
	{
		MessageComposer mc(env, options_cache, m, oncomposefield, onmessagebegin, onmessageend);

		if (verbosity >= 2)
		{
			// packet input fields
			for (int i = 0; i < env->packet_root_variable->packet.fields_size(); i++)
			{
				pkt2::Field f = env->packet_root_variable->packet.fields(i);
			}
			// packet output variables
			for (int i = 0; i < env->packet_root_variable->fieldname_variables.size(); i++)
			{
				FieldNameVariable v = env->packet_root_variable->fieldname_variables[i];
			}
		}
	}

	return m;
}

google::protobuf::Message *Packet2Message::getMessageByName
(
	const std::string &messageType
)
{
	return declarations->getMessage(messageType);
}
