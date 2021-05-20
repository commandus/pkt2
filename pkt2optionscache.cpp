/*
 * pkt2optionscache.cpp
 */
#include <vector>
#include <stdlib.h>
#ifdef ENABLE_LOG
#include <glog/logging.h>
#endif
#include "pkt2optionscache.h"
#include "bin2ascii.h"

Pkt2OptionsCache::Pkt2OptionsCache() 
	: protobuf_decrarations(NULL)
{
}

Pkt2OptionsCache::Pkt2OptionsCache
(
	ProtobufDeclarations *protobufdeclarations
)
	: protobuf_decrarations(protobufdeclarations)
{
	addDeclarations(protobufdeclarations);
}

Pkt2OptionsCache::~Pkt2OptionsCache() 
{
}

void Pkt2OptionsCache::addDeclarations
(
	ProtobufDeclarations *protobuf_declarations
)
{
	std::map<std::string, const google::protobuf::Descriptor*> *m = protobuf_declarations->getMessages();

	for (std::map<std::string, const google::protobuf::Descriptor*>::iterator it = m->begin(); it != m->end(); ++it)
	{
		pkt2packet_variable[it->first] = Pkt2PacketVariable(it->first, protobuf_declarations);
	}
}

int Pkt2OptionsCache::size_of
(
		enum pkt2::OutputType t
)
{
	switch (t) {
	case pkt2::OUTPUT_NONE:
		return 0;
	case pkt2::OUTPUT_DOUBLE:
	case pkt2::OUTPUT_INT64:
	case pkt2::OUTPUT_UINT64:
	case pkt2::OUTPUT_FIXED64:
	case pkt2::OUTPUT_SFIXED64:
	case pkt2::OUTPUT_SINT64:
		return 8;
	case pkt2::OUTPUT_FLOAT:
	case pkt2::OUTPUT_INT32:
	case pkt2::OUTPUT_FIXED32:
	case pkt2::OUTPUT_SFIXED32:
	case pkt2::OUTPUT_UINT32:
	case pkt2::OUTPUT_SINT32:
		return 4;
	case pkt2::OUTPUT_BOOL:
	case pkt2::OUTPUT_ENUM:
		return 1;
	case pkt2::OUTPUT_STRING:
	case pkt2::OUTPUT_GROUP:
	case pkt2::OUTPUT_MESSAGE:
	case pkt2::OUTPUT_BYTES:
		return 0;
	default:
		return 0;
	}
}

/**
 * @brief Find out PacketVariable by full message type name (Protobuf_packet_name.message_type)
 * @param message Protobuf full type name (with packet name)
 * @param found return value true- found
 * @return found PacketVariable
 */
const Pkt2PacketVariable &Pkt2OptionsCache::getPacketVariable
(
	const std::string &message_type,
	bool *found
) const
{
	std::map<std::string, Pkt2PacketVariable>::const_iterator m = pkt2packet_variable.find(message_type);
	if (found) {
		*found = (m != pkt2packet_variable.end());
	}
	return m->second;
}

/**
 * @brief Find out first found PacketVariable by size and tag
 * @param packet packet data
 * @param found return value true- found
 * @return found PacketVariable
 */
const Pkt2PacketVariable &Pkt2OptionsCache::find1
(
		const std::string &packet,
		bool *found
)
{
	std::string message_type;
	if (found)
		*found = false;
	for (std::map<std::string, Pkt2PacketVariable>::const_iterator m(pkt2packet_variable.begin());
			m != pkt2packet_variable.end(); ++m)
	{

		if (validTags(m->first, m->second, packet))
		{
			message_type = m->first;
			if (found)
				*found = true;
#ifdef ENABLE_LOG		
			LOG(INFO) << "Found " << message_type;
#endif				
			break;
		}
	}
	return pkt2packet_variable[message_type];
}

/**
 * @brief validate tags
 * @param message_type packet.message
 * @param var Pkt2PacketVariable 
 * @param packet packet data
 * @return true if all tags found
 */
bool Pkt2OptionsCache::validTags
(
	const std::string &message_type,
	const Pkt2PacketVariable &var,
	const std::string &packet
)
{
	if (!var.validTags(packet))
		return false;
	const google::protobuf::Descriptor *m = protobuf_decrarations->getMessageDescriptor(message_type);
	if (!m)
		return true;
	for (int f = 0; f < m->field_count(); f++)
	{
		if (m->field(f)->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
			continue;
		
		// get a "zone" inside a packet corresponding to the message in the entire data
		const FieldNameVariable *fnv = var.getVariableByFieldNumber(m->field(f)->number());
		if (!fnv) 
			continue;
		// extract "zone" (field in the parent message) by name in the variable's "get" property.
		// get can be: "field.name" (preferred) or "name" (optional)
		std::string zone = fnv->var.get();
		// if "get" contains "field." prefix, remove it
		std::string::size_type pos = zone.find("field.");
		if (pos != std::string::npos)
			zone = zone.substr(pos + 6);

		// Search field in the parent packet fields by the name
		int z = -1;
		for (int j = 0; j < var.packet.fields_size(); j++)
		{
			if (var.packet.fields(j).name() == zone)
			{
				z = j;
				break;
			}
		}
		if (z < 0)
			continue;

		// extract field from the parent packet
		const std::string &subpacket = packet.substr(var.packet.fields(z).offset(), var.packet.fields(z).size());
		std::map<std::string, Pkt2PacketVariable>::const_iterator it = pkt2packet_variable.find(m->field(f)->message_type()->full_name());
		if (it == pkt2packet_variable.end())
			continue;

		if (!validTags(m->field(f)->full_name(), it->second, subpacket))
			return false;
		
	}
	return true;
	
}

/**
 * Check if field have index
 * @param message_type message name
 * @param field_type field name
 * @return 0- no index, 1,2.. index
 */
int Pkt2OptionsCache::getIndex(
		const std::string &message_type,
		const std::string &field_type
) const
{
	std::map<std::string, Pkt2PacketVariable>::const_iterator it = pkt2packet_variable.find(message_type);
	if (it == pkt2packet_variable.end())
		return 0;

	for (int i = 1; i < it->second.keyIndexes.size(); i++)
	{
		if (field_type == it->second.fieldname_variables[it->second.keyIndexes[i]].field_name)
			return i;
	}
	return 0;
}

#define PUSH_KEY(TYPE, PROTO_CPP_TYPE) \
{ \
	TYPE v = ref->Get ## PROTO_CPP_TYPE(*message, field); \
	memmove(&(((char*)buffer)[r]), &v, sizeof(v)); \
	r += sizeof(v); \
}

/**
 * get values from message fields with index 1, 2... into key buffer
 * @param messageType
 * @param buffer
 * @param max_size
 * @param message
 * @return
 */
size_t Pkt2OptionsCache::getKey(
	const std::string &messageType,
	void *buffer,
	size_t max_size,
	const google::protobuf::Message *message
)
{
	Pkt2PacketVariable pv(pkt2packet_variable[messageType]);

	size_t r = 0;
	std::vector<uint64_t>::iterator it(pv.keyIndexes.begin());
	// first is identifier (or hash) of the message
	if (it != pv.keyIndexes.end())
	{
		uint64_t t = *it;
		memmove(buffer, &t, sizeof(uint64_t));
		r+= sizeof(uint64_t);
		it++;
	}
	for (;it != pv.keyIndexes.end(); ++it)
	{
		pkt2::Variable v = pkt2packet_variable[messageType].fieldname_variables[*it].var;
		const google::protobuf::FieldDescriptor *field = message->GetDescriptor()->field(*it);
		const google::protobuf::Reflection *ref = message->GetReflection();
        if (!field)
            return 0;
		google::protobuf::FieldDescriptor::CppType t = field->cpp_type();
		switch (t) 
		{
	        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
	        	PUSH_KEY(double, Double)
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
	        	PUSH_KEY(float, Float)
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
	        	PUSH_KEY(int64_t, Int64)
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
	        	PUSH_KEY(uint64_t, UInt64)
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
	        	PUSH_KEY(int32_t, Int32)
				break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
	        	PUSH_KEY(uint32_t, UInt32)
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
	        	PUSH_KEY(bool, Bool)
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
	        {
	            bool is_binary = field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES;
                std::string value = ref->GetString(*message, field);
                if (is_binary)
                    value = b64_encode(value);
				memmove(&(((char*)buffer)[r]), value.c_str(), value.size());
				r += value.size();
	            break;
	        }
	        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
	        	{
					const google::protobuf::EnumValueDescriptor* value = ref->GetEnum(*message, field);
					int v = value->number();
					memmove(&(((char*)buffer)[r]), &v, sizeof(v));
	        	}
	            break;
	        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
	        default:
	            break;
	    }
	}
	return r;
}

/**
	 * Return message identifier (or message name hash if id is not assigned)
	 * @param messageType
	 * @return
	 */
uint64_t Pkt2OptionsCache::Pkt2OptionsCache::getMessageId
(
	const std::string &messageType
)
{
	Pkt2PacketVariable pv(pkt2packet_variable[messageType]);
	std::vector<uint64_t>::iterator it(pv.keyIndexes.begin());
	if (it != pv.keyIndexes.end())
		return *it;
	else
		return 0;

}

