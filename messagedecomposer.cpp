/*
 * messagedecomposer.cpp
 *
 *  Created on: Mar 10, 2017
 *      Author: andrei
 */

#include <sstream>
#include "messagedecomposer.h"
#include "errorcodes.h"
#include "bin2ascii.h"

MessageDecomposer::MessageDecomposer()
	: env(NULL), ondecompose(NULL), options_cache(NULL)
{
}

MessageDecomposer::~MessageDecomposer() {
}

MessageDecomposer::MessageDecomposer
(
		void *environment,
		Pkt2OptionsCache *options,
		const google::protobuf::Message *message,
		ondecompose_callback callback
)
	: env(environment), ondecompose(callback), options_cache(options)
{
	decompose(message);
}

void MessageDecomposer::setCallback
(
		ondecompose_callback ondecompose
)
{
	this->ondecompose = ondecompose;
}

#define DECOMPOSE_TYPE(NATIVE_TYPE, PROTO_TYPE) \
{ \
	NATIVE_TYPE value; \
	if (repeated) \
	{ \
		for (size_t i = 0; i != array_size; ++i) \
		{ \
			value = ref->GetRepeated ## PROTO_TYPE(*message, field, i); \
			ondecompose(env, message_descriptor, t, field->name(), &value, sizeof(NATIVE_TYPE), index); \
		} \
	} \
	else \
	{ \
		value = ref->Get ## PROTO_TYPE(*message, field); \
		ondecompose(env, message_descriptor, t, field->name(), &value, sizeof(NATIVE_TYPE), index); \
	} \
}

/**
 * call callback here
 * @param message_descriptor
 * @param message
 * @param field
 * @return
 */
int MessageDecomposer::decomposeField
(
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::Message *message,
	const google::protobuf::FieldDescriptor *field
)
{
    const google::protobuf::Reflection *ref = message->GetReflection();
    const bool repeated = field->is_repeated();

    size_t array_size = 0;
    if (repeated)
        array_size = ref->FieldSize(*message, field);

    int index = 0;
    google::protobuf::FieldDescriptor::CppType t = field->cpp_type();

    if (options_cache)
    {
    	// check is it index
    	index = options_cache->getIndex(message_descriptor->full_name(), field->name());
    }

    switch (t)
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        	DECOMPOSE_TYPE(double, Double)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        	DECOMPOSE_TYPE(float, Float)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        	DECOMPOSE_TYPE(int64_t, Int64)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        	DECOMPOSE_TYPE(uint64_t, UInt64)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        	DECOMPOSE_TYPE(int32_t, Int32)
			break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        	DECOMPOSE_TYPE(uint32_t, UInt32)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        	DECOMPOSE_TYPE(bool, Bool)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            bool is_binary = field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES;
            if (repeated)
            {
                for (size_t i = 0; i != array_size; ++i)
                {
                    std::string value = ref->GetRepeatedString(*message, field, i);
                    if (is_binary)
                        value = b64_encode(value);
                    ondecompose(env, message_descriptor, t, field->name(), (void *) value.c_str(), value.length(), index);
                }
            }
            else
            {
                std::string value = ref->GetString(*message, field);
                if (is_binary)
                {
                    value = b64_encode(value);
                }
                ondecompose(env, message_descriptor, t, field->name(), (void *) value.c_str(), value.length(), index);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            if (repeated)
            {
                for (size_t i = 0; i != array_size; ++i)
                {
                    const google::protobuf::Message *value = &(ref->GetRepeatedMessage(*message, field, i));
                    decompose(value);
                }
            }
            else
            {
                const google::protobuf::Message *value = &(ref->GetMessage(*message, field));
                decompose(value);
            }
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        	{
        		int v;
				if (repeated)
				{
					for (size_t i = 0; i != array_size; ++i)
					{
						const google::protobuf::EnumValueDescriptor* value = ref->GetRepeatedEnum(*message, field, i);
						v = value->number();
						ondecompose(env, message_descriptor, t, field->name(), &v, sizeof(v), index);
					}
				}
				else
				{
					const google::protobuf::EnumValueDescriptor* value = ref->GetEnum(*message, field);
					v = value->number();
					ondecompose(env, message_descriptor, t, field->name(), &v, sizeof(v), index);
				}
        	}
            break;
        default:
            break;
    }
	return ERR_OK;	
}

int MessageDecomposer::decompose
(
	const google::protobuf::Message *message
)
{
	if (!ondecompose)
		return ERRCODE_NO_CALLBACK;
    const google::protobuf::Descriptor *message_descriptor = message->GetDescriptor();
    if (!message_descriptor)
        return ERRCODE_DECOMPOSE_NO_MESSAGE_DESCRIPTOR;
    const google::protobuf::Reflection *ref = message->GetReflection();
    if (!ref)
        return ERRCODE_DECOMPOSE_NO_REFECTION;
    size_t count = message_descriptor->field_count();
    for (size_t i = 0; i != count; ++i)
    {
        const google::protobuf::FieldDescriptor *field = message_descriptor->field(i);
        if (!field)
            return ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR;
       	decomposeField(message_descriptor, message, field);
    }
    return ERR_OK;
}

#define TO_STRING(NATIVE_TYPE) \
{ \
	NATIVE_TYPE val; \
	val = *((NATIVE_TYPE *) value); \
	ss << val; \
}

/**
  * @brief return human readable value as string
  */
std::string MessageDecomposer::toString
(
	google::protobuf::FieldDescriptor::CppType field_type,
	void* value,
	int size
)
{
	std::stringstream ss;
    switch (field_type)
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        	TO_STRING(double)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        	TO_STRING(float)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        	TO_STRING(int64_t)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        	TO_STRING(uint64_t)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        	TO_STRING(int32_t)
			break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        	TO_STRING(uint32_t)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        	TO_STRING(bool)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
       		ss << std::string((char *) value, size);
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        	ss << "Message " << ERR_NOT_IMPLEMENTED;
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        	TO_STRING(int)
            break;
        default:
            break;
    }

	return ss.str();
}
