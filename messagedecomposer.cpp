/*
 * messagedecomposer.cpp
 *
 *  Created on: Mar 10, 2017
 *      Author: andrei
 */

#include "messagedecomposer.h"
#include "errorcodes.h"
#include "bin2ascii.h"

MessageDecomposer::MessageDecomposer()
	: ondecompose(NULL)
{
}

MessageDecomposer::~MessageDecomposer() {
}

MessageDecomposer::MessageDecomposer
(
	const google::protobuf::Message *message
)
	: ondecompose(NULL)
{
	decompose(message);
}

#define DECOMPOSE_TYPE(NATIVE_TYPE, PROTO_TYPE) \
{ \
	NATIVE_TYPE value; \
	if (repeated) \
	{ \
		for (size_t i = 0; i != array_size; ++i) \
		{ \
			value = ref->GetRepeated ## PROTO_TYPE(*message, field, i); \
			ondecompose(t, &value, sizeof(NATIVE_TYPE)); \
		} \
	} \
	else \
	{ \
		value = ref->Get ## PROTO_TYPE(*message, field); \
		ondecompose(t, &value, sizeof(NATIVE_TYPE)); \
	} \
}

int MessageDecomposer::decomposeField
(
	const google::protobuf::Message *message,
	const google::protobuf::FieldDescriptor *field
)
{
    const google::protobuf::Reflection *ref = message->GetReflection();
    const bool repeated = field->is_repeated();

    size_t array_size = 0;
    if (repeated)
        array_size = ref->FieldSize(*message, field);

    google::protobuf::FieldDescriptor::CppType t = field->cpp_type();
    switch (field->cpp_type())
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
                    ondecompose(t, (void *) value.c_str(), value.length());
                }
            }
            else
            {
                std::string value = ref->GetString(*message, field);
                if (is_binary)
                {
                    value = b64_encode(value);
                }
                ondecompose(t, (void *) value.c_str(), value.length());
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
						ondecompose(t, &v, sizeof(v));
					}
				}
				else
				{
					const google::protobuf::EnumValueDescriptor* value = ref->GetEnum(*message, field);
					v = value->number();
					ondecompose(t, &v, sizeof(v));
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
    const google::protobuf::Descriptor *d = message->GetDescriptor();
    if (!d)
        return ERRCODE_DECOMPOSE_NO_MESSAGE_DESCRIPTOR;
    const google::protobuf::Reflection *ref = message->GetReflection();
    if (!ref)
        return ERRCODE_DECOMPOSE_NO_REFECTION;
    size_t count = d->field_count();
    for (size_t i = 0; i != count; ++i)
    {
        const google::protobuf::FieldDescriptor *field = d->field(i);
        if (!field)
            return ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR;
        if (!(field->is_optional() && !ref->HasField(*message, field)))
        	decomposeField(message, field);
    }
    return ERR_OK;
}
