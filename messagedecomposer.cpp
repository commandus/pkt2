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
#include "javascript-context.h"
#include "pkt2.pb.h"

MessageDecomposer::MessageDecomposer()
	: env(NULL), ondecompose(NULL), options_cache(NULL)
{
	context = getFormatJavascriptContext();
}

MessageDecomposer::~MessageDecomposer()
{
	if (context != NULL)
	{
		duk_pop(context);
		duk_destroy_heap(context);
	}
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
	// Create Javascript context with global object field.xxx
	// duk_context *jsCtx = getFormatJavascriptContext();

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
			ondecompose(this, env, message_descriptor, field, &value, sizeof(NATIVE_TYPE), index); \
		} \
	} \
	else \
	{ \
		value = ref->Get ## PROTO_TYPE(*message, field); \
		ondecompose(this, env, message_descriptor, field, &value, sizeof(NATIVE_TYPE), index); \
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
                    ondecompose(this, env, message_descriptor, field, (void *) value.c_str(), value.length(), index);
                }
            }
            else
            {
                std::string value = ref->GetString(*message, field);
                if (is_binary)
                {
                    value = b64_encode(value);
                }
                ondecompose(this, env, message_descriptor, field, (void *) value.c_str(), value.length(), index);
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
						ondecompose(this, env, message_descriptor, field, &v, sizeof(v), index);
					}
				}
				else
				{
					const google::protobuf::EnumValueDescriptor* value = ref->GetEnum(*message, field);
					v = value->number();
					ondecompose(this, env, message_descriptor, field, &v, sizeof(v), index);
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
    /*
    const google::protobuf::Reflection *ref = message->GetReflection();
    if (!ref)
       return ERRCODE_DECOMPOSE_NO_REFECTION;
       */
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
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor *field,
	const void* value,
	int size,
	int format_number
)
{
	std::stringstream ss;
    switch (field->cpp_type())
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

	return format(message_descriptor, field, ss.str(), format_number);
}

/**
  * @brief format
  */
std::string MessageDecomposer::format
(
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor *field,
	const std::string& value,
	int format_number
)
{
	bool found;

	std::string n = message_descriptor->full_name();
	const Pkt2PacketVariable &v = options_cache->getPacketVariable(n, &found);
	if (!found)
		return value;

	const FieldNameVariable* f = v.getVariableByFieldNumber(field->number());
	if (!f)
		return value;


	if (format_number < f->var.format_size())
	{
		// use format
		std::string fmt = f->var.format(format_number);
		return fmt + " " + value;
	}

	return value;

	/*
	const Pkt2PacketVariable &pv = (!force_message.empty()
			? options_cache->getPacketVariable(force_message, &found)
					: options_cache->find1(packet, &found));

	if (!found)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << force_message;
		return NULL;
	}

	 */
}

/*
google::protobuf::Message *Packet2Message::parse
(
    struct sockaddr *socket_address_src,
    struct sockaddr *socket_address_dst,
	const std::string &packet,
	const std::string &force_message
)
{
	PacketParseEnvironment env(&pv, jsCtx);
	google::protobuf::Message *m = declarations->getMessage(pv.message_type);
	if (m)
	{
		MessageComposer mc(&env, options_cache, m, oncomposefield, onnextmessage);

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
				LOG(INFO) << m->GetTypeName() << "." << v.field_name << " = " << extractVariable(jsCtx, v) << " " << v.var.measure_unit() << " (" << v.var.full_name() << ")";
			}
		}
	}

	duk_pop(jsCtx);
	duk_destroy_heap(jsCtx);

	return m;
}
*/
