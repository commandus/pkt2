/*
 * pkt2optionscache.cpp
 */

#include <vector>
#include <stdlib.h>
#include "pkt2optionscache.h"
#include "bin2ascii.h"

Pkt2OptionsCache::Pkt2OptionsCache() {
}

Pkt2OptionsCache::Pkt2OptionsCache(ProtobufDeclarations *protobuf_declarations)
{
	addDeclarations(protobuf_declarations);
}

Pkt2OptionsCache::~Pkt2OptionsCache() {
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

const Pkt2PacketVariable &Pkt2OptionsCache::getPacketVariable
(
	const std::string &message_type,
	bool *found
)
{
	std::map<std::string, Pkt2PacketVariable>::iterator m = pkt2packet_variable.find(message_type);
	if (found)
		*found = (m != pkt2packet_variable.end());
	return pkt2packet_variable[message_type];
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
)
{
	Pkt2PacketVariable pv(pkt2packet_variable[message_type]);

	for (int i = 1; i < pv.keyIndexes.size(); i++)
	{
		if (field_type == pv.fieldname_variables[pv.keyIndexes[i]].field_name)
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

