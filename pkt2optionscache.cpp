/*
 * pkt2optionscache.cpp
 */

#include <vector>
#include <stdlib.h>
#include "pbjson.hpp"
#include "pkt2optionscache.h"

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
		Pkt2PacketVariable pv(it->first, protobuf_declarations);
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
	Pkt2PacketVariable pv(pkt2[message_type]);
	for (int i = 1; i < pv.keyIndexes.size(); i++)
	{
		if (field_type == pv.variables[i].name())
			return i;
	}
	return 0;
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
	Pkt2PacketVariable pv(pkt2[messageType]);

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

    rapidjson::Value *js = pbjson::pb2jsonobject(message);
	for (;it != pv.keyIndexes.end(); ++it)
	{
		pkt2::Variable v = pkt2[messageType].variables[*it];
		const google::protobuf::FieldDescriptor *field = message->GetDescriptor()->field(*it);
        if (!field)
            return 0;

        rapidjson::Value::ConstMemberIterator itr = js->FindMember(field->name().c_str());
        if (itr == js->MemberEnd())
        	return 0;
        switch (itr->value.GetType()) {
			case rapidjson::kNumberType:
				if (itr->value.IsInt64())
				{
					int64_t v = itr->value.GetInt64();
					memmove(&(((char*)buffer)[r]), &v, sizeof(v));
					r += sizeof(v);
				} else
					if (itr->value.IsUint64())
					{
						uint64_t v = itr->value.GetUint64();
						memmove(&(((char*)buffer)[r]), &v, sizeof(v));
						r += sizeof(v);
					} else
						if (itr->value.IsInt())
						{
							int v = itr->value.GetInt();
							memmove(&(((char*)buffer)[r]), &v, sizeof(v));
							r += sizeof(v);
						} else
							if (itr->value.IsUint())
							{
								uint v = itr->value.GetUint();
								memmove(&(((char*)buffer)[r]), &v, sizeof(v));
								r += sizeof(v);
							} else
								if (itr->value.IsDouble())
								{
									double v = itr->value.GetDouble();
									memmove(&(((char*)buffer)[r]), &v, sizeof(v));
									r += sizeof(v);
								} else
									if (itr->value.IsFloat())
									{
										float v = itr->value.GetFloat();
										memmove(&(((char*)buffer)[r]), &v, sizeof(v));
										r += sizeof(v);
									} else
										if (itr->value.IsBool())
										{
											bool v = itr->value.GetBool();
											memmove(&(((char*)buffer)[r]), &v, sizeof(v));
											r += sizeof(v);
										}
				break;
			default:
				break;
        }
        delete js;
	}
	return r;
}

