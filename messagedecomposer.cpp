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
#include "google/protobuf/reflection.h"
#include "pkt2.pb.h"
#include "pkt2packetvariable.h"

using namespace google::protobuf;

MessageDecomposer::MessageDecomposer()
	: env(NULL), ondecompose(NULL), options_cache(NULL), context(NULL)
{
}

MessageDecomposer::~MessageDecomposer()
{
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
    FieldDescriptor::CppType t = field->cpp_type();

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

#define CASE_CPP_TYPE(CTX, MSG, FLD, REF, CPP_TYP, DUK_TYP, PRT_TYP) \
	case FieldDescriptor::CPP_TYP: \
		if (FLD->is_repeated()) \
		{ \
			int arr_idx = duk_push_array(context); \
			for (size_t i = 0; i != REF->FieldSize(*MSG, FLD); ++i) \
			{ \
				duk_push_ ## DUK_TYP(CTX, REF->GetRepeated ## PRT_TYP(*MSG, FLD, i)); \
				duk_put_prop_index(CTX, arr_idx, i); \
			} \
		} \
		else \
		{ \
			duk_push_ ## DUK_TYP(CTX, REF->Get ## PRT_TYP(*MSG, FLD)); \
		} \
		duk_put_prop_string(context, -2, FLD->name().c_str()); \
		break; \

/**
 * Add "message" javascript object to the context
 * @param message
 */
void MessageDecomposer::addJavascriptMessage
(
	const google::protobuf::Message *message,
	const google::protobuf::Message *root_message
)
{
	const Descriptor *d = message->GetDescriptor();
	if (!d)
		return;
	duk_push_object(context);

	size_t count = d->field_count();
	for (size_t i = 0; i != count; ++i)
	{
		const FieldDescriptor *field = d->field(i);
		if (!field)
			return;
		const Reflection *ref = message->GetReflection();
		if (!ref)
			return;
		if (field->is_optional() && !ref->HasField(*message, field))
		{
			//do nothing
		}
		else
		{
			addJavascriptField(message, root_message, field);
		}
	}
	return;
}

/**
 * Add "message.<field>" javascript object to the context
 * @param message
 */
void MessageDecomposer::addJavascriptField
(
	const google::protobuf::Message *message,
	const google::protobuf::Message *root_message,
	const google::protobuf::FieldDescriptor *field
)
{
	const google::protobuf::Reflection *ref = message->GetReflection();
	switch (field->cpp_type())
	{
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_DOUBLE, number, Double)
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_FLOAT, number, Float)
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_INT64, int, Int64)
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_UINT64, uint, UInt64)
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_INT32, int, Int32)
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_UINT32, uint, UInt32)
		CASE_CPP_TYPE(context, message, field, ref, CPPTYPE_BOOL, boolean, Bool)
		case FieldDescriptor::CPPTYPE_STRING:
			{
				bool is_binary = field->type() == FieldDescriptor::TYPE_BYTES;
				if (field->is_repeated())
				{
					int arr_idx = duk_push_array(context);
					for (size_t i = 0; i != ref->FieldSize(*message, field); ++i)
					{
						std::string value = ref->GetRepeatedString(*message, field, i);
						if (is_binary)
							value = b64_encode(value);
						duk_push_string(context, ref->GetRepeatedString(*message, field, i).c_str());
						duk_put_prop_index(context, arr_idx, i);
					}
				}
				else
				{
					std::string value = ref->GetString(*message, field);
					if (is_binary)
						value = b64_encode(value);
					duk_push_string(context, ref->GetString(*message, field).c_str());
				}
				duk_put_prop_string(context, -2, field->name().c_str());
			}
			break;
		case FieldDescriptor::CPPTYPE_ENUM:
			if (field->is_repeated())
			{
				int arr_idx = duk_push_array(context);
				for (size_t i = 0; ref->FieldSize(*message, field); ++i)
				{
					duk_push_int(context, ref->GetRepeatedEnum(*message, field, i)->number());
					duk_put_prop_index(context, arr_idx, i);
				}
			}
			else
				duk_push_int(context, ref->GetEnum(*message, field)->number());
			duk_put_prop_string(context, -2, field->name().c_str());
			break;
		case FieldDescriptor::CPPTYPE_MESSAGE:
			if (field->is_repeated())
			{
				int arr_idx = duk_push_array(context);
				for (size_t i = 0; i != ref->FieldSize(*message, field); ++i)
				{
					const Message *value = &(ref->GetRepeatedMessage(*message, field, i));
					duk_push_object(context);
			    	addJavascriptMessage(value, root_message);
					duk_put_prop_index(context, arr_idx, i);
				}
			}
			else
			{
				const Message *value = &(ref->GetMessage(*message, field));
				duk_push_object(context);
		    	addJavascriptMessage(value, root_message);
			}
			duk_put_prop_string(context, -2, field->name().c_str());
			break;
		default:
			break;
	}
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

	context = getFormatJavascriptContext((void *) message);
	// value
	duk_push_global_object(context);
	addJavascriptMessage(message, message);
	duk_put_global_string(context, "value");

    for (size_t i = 0; i != message_descriptor->field_count(); ++i)
    {
        const google::protobuf::FieldDescriptor *field = message_descriptor->field(i);
        if (!field)
            return ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR;
       	decomposeField(message_descriptor, message, field);
    }

	duk_pop(context);
	duk_destroy_heap(context);

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
		duk_eval_string(context, f->var.format(format_number).c_str());
		return std::string(duk_safe_to_string(context, -1));
	}

	return value;
}
