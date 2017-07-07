/*
 * messagedecomposer.cpp
 */

#include <iostream>
#include <sstream>
#include "messagedecomposer.h"
#include "errorcodes.h"
#include "bin2ascii.h"
#include "javascript-context.h"
#include "google/protobuf/reflection.h"
#include "pkt2.pb.h"
#include "pkt2packetvariable.h"
#include "javascript-context.h"
#include "utilprotobuf.h"

using namespace google::protobuf;

#define	DUK_DEBUG(context) \
	duk_push_context_dump(context); \
	std::cout << duk_to_string(context, -1) << std::endl; \
	duk_pop(context);


/**
	* @brief message decompser
	* @param context can be NULL
	*/
MessageDecomposer::MessageDecomposer
(
	void *environment, 
	JavascriptContext *acontext,
	Pkt2OptionsCache *options, 
	const google::protobuf::Message *message, 
	ondecompose_callback callback
)
	: env(environment), ondecompose(callback), options_cache(options)
{
	if (acontext)
	{
		context = acontext;
		own_context = false;
	}
	else
	{
		context = new JavascriptContext;
		own_context = true;
	}

	// value
	duk_push_global_object(context->context);
	addJavascriptMessage(context->context, message);
	duk_put_prop_string(context->context, -2, "value");
	duk_pop(context->context);

	decompose(message);
}

/**
  * @brief message decompser
  */
MessageDecomposer::MessageDecomposer
(
	void *environment, 
	const std::string &messagetypename,
	Pkt2OptionsCache *options, 
	const google::protobuf::Message *message, 
	ondecompose_callback callback
)
	: env(environment), own_context(true), ondecompose(callback), options_cache(options)
{
	Pkt2PacketVariable packet_root_variable(messagetypename, options->protobuf_decrarations);
	std::string packet = packet_root_variable.getEmptyPacket();
	context = new JavascriptContext (options, &packet_root_variable, packet);
	// message = options->protobuf_decrarations->getMessage(messagetypename);
	// value
	duk_push_global_object(context->context);
	MessageDecomposer::addJavascriptMessage(context->context, message);
	duk_put_prop_string(context->context, -2, "value");
	duk_pop(context->context);

	decompose(message);
}

MessageDecomposer::~MessageDecomposer()
{
	if (own_context && context)
		delete context;
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
 * @brief decompose Protobuf message field using callback
 * @param message_descriptor Protobuf message descruiptor
 * @param message Protobuf message to decompose
 * @param field field descriptor
 * @return 0 success
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
        	}            break;

        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            if (repeated)
            {
                for (size_t i = 0; i != array_size; ++i)
                {
                    const google::protobuf::Message *value = &(ref->GetRepeatedMessage(*message, field, i));
                    decomposeMessage(value);
                }
            }
            else
            {
                const google::protobuf::Message *value = &(ref->GetMessage(*message, field));
                decomposeMessage(value);
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
 * @brief Add "message" javascript object to the context
 * @param message Protobuf message to add
 */
void MessageDecomposer::addJavascriptMessage
(
	duk_context *context,
	const google::protobuf::Message *message
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
			addJavascriptFieldDefault(context, message, field);
			continue;
		}
		addJavascriptField(context, message, field);
	}
	return;
}

/**
 * @brief Add "message.field" javascript object to the context
 * @param message
 */
void MessageDecomposer::addJavascriptField
(
	duk_context *context,
	const google::protobuf::Message *message,
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
						duk_push_string(context, value.c_str());
						duk_put_prop_index(context, arr_idx, i);
					}
				}
				else
				{
					std::string value = ref->GetString(*message, field);
					if (is_binary)
						value = b64_encode(value);
					duk_push_string(context, value.c_str());
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
			    	addJavascriptMessage(context, value);
					duk_put_prop_index(context, arr_idx, i);
				}
			}
			else
			{
				const Message *value = &(ref->GetMessage(*message, field));
		    	addJavascriptMessage(context, value);
			}
			duk_put_prop_string(context, -2, field->name().c_str());
			break;
		default:
			break;
	}
}

/**
 * @brief Add "message.field" javascript object to the context
 * @param message
 */
void MessageDecomposer::addJavascriptFieldDefault
(
	duk_context *context,
	const google::protobuf::Message *message,
	const google::protobuf::FieldDescriptor *field
)
{
	// TODO: No difference between addJavascriptField() and addJavascriptFieldDefault()?
	return addJavascriptField(context, message, field);
}

int MessageDecomposer::decomposeMessage
(
	const google::protobuf::Message *message
)
{
    const google::protobuf::Descriptor *message_descriptor = message->GetDescriptor();
    if (!message_descriptor)
        return ERRCODE_DECOMPOSE_NO_MESSAGE_DESCRIPTOR;

    for (size_t i = 0; i != message_descriptor->field_count(); ++i)
    {
        const google::protobuf::FieldDescriptor *field = message_descriptor->field(i);
        if (!field)
            return ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR;
       	decomposeField(message_descriptor, message, field);
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
	decomposeMessage(message);
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
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        	TO_STRING(int)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
       		ss << std::string((char *) value, size);
            break;
        }
        // case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        default:
            break;
    }

    if (format_number >= 0)
		return format(message_descriptor, field, ss.str(), format_number);
	else
		return ss.str();
}

/**
  * @brief format using format function (if provided)
  * @param message_descriptor Protobuf message descriptor
  * @param field Protobuf field descriptor
  * @param value value to be formatted
  * @param format_number format index
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
		std::string expr = f->var.format(format_number);
		context->expression = &expr;
		if (duk_peval_string(context->context, expr.c_str()) != 0)
		{
			// TODO error report
		}
		return std::string(duk_safe_to_string(context->context, -1));
	}

	return value;
}

/**
  * @brief set "output" fields in the packet
  * @param field Protobuf field descriptor
  * @param value value to set in human readable text representation
  */
void MessageDecomposer::setJsValue
(
	duk_context *context,
	const google::protobuf::FieldDescriptor *field,
	const std::string& value
)
{
	std::string expr = "value." + field->name() + " = " + value;
	// context->expression = &expr;
	if (duk_peval_string(context, expr.c_str()) != 0)
	{
		expr = "value." + field->name() + " = \"" + value + "\"";
		// context->expression = &expr;
		if (duk_peval_string(context, expr.c_str()) != 0)
		{
			// TODO error report
		}
		duk_pop(context);
	}
	duk_pop(context);
}

/**
  * @brief get "input" fields in the packet
  * @param field_name field name
  * @return value in human readable text representation
  */
std::string MessageDecomposer::getJsField
(
	duk_context *context,
	const std::string &field_name
)
{
	std::string expr = "field." + field_name;
	if (duk_peval_string(context, expr.c_str()) != 0)
	{
		// TODO error report
	}
	std::string r(duk_safe_to_string(context, -1));
	duk_pop(context);
	return r;
}

/**
  * @brief set "input" fields in the packet
  * @param packet where set bits
  * @param message_descriptor Protobuf message descriptor
  * @param field Protobuf field descriptor
  * @param value value to set in human readable text representation
  */
void MessageDecomposer::setFieldsByVariable
(
	duk_context *context,
	Pkt2OptionsCache *options_cache,
	Pkt2PacketVariable *packetVariable,
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor *field,
	const std::string& value
)
{
	bool found;
	std::string n = message_descriptor->full_name();
	const Pkt2PacketVariable &v = options_cache->getPacketVariable(n, &found);
	if (!found)
		return;
	const FieldNameVariable* f = v.getVariableByFieldNumber(field->number());
	if (!f)
		return;
	
	// set output (value.)
	setJsValue(context, field, value);

	// set input (field.f1, field2..)
	std::string expr = f->var.set();
	// context->expression = &expr;
	if (duk_peval_string(context, expr.c_str()) != 0) 
	{
	}
	const char * r = duk_safe_to_string(context, -1);
	duk_pop(context);
// DUK_DEBUG(context)	
}

std::string MessageDecomposer::reflectJsFieldsToMessagePacketField
(
	const std::string& packet,
	duk_context *context,
	Pkt2OptionsCache *options_cache,
	const pkt2::Field &field
)
{
	std::string r = packet;
	std::string vs = getJsField(context, field.name());
	
	switch (field.type()) {
		case pkt2::INPUT_MESSAGE:
		case pkt2::INPUT_NONE:
			break;
		case pkt2::INPUT_CHAR:
		case pkt2::INPUT_STRING:
			setFieldString(r, field, vs);
			break;
		case pkt2::INPUT_DOUBLE:
			{
				double v = strtod(vs.c_str(), NULL);
				setFieldDouble(r, field, v);
			}
			break;
		case pkt2::INPUT_BYTES:
		{
			int c = 0;
			int sz = field.offset() + field.size();
			for (int i = field.offset(); i < sz; i++)
			{
				r[i];
				c++;
			}
			break;
		}
		default:
			{
				uint64_t v = strtoul(vs.c_str(), NULL, 10);
				setFieldUInt(r, field, v);
			}
			break;
	}
	return r;
}

 std::string MessageDecomposer::reflectJsFieldsToMessagePacket
(
	const std::string& packet,
	duk_context *context,
	Pkt2OptionsCache *options_cache,
	const Pkt2PacketVariable *packet_variable
)
{
	std::string r = packet;
	// put non-message fields
	for (int i = 0; i < packet_variable->packet.fields_size(); i++)
	{
		const pkt2::Field &f = packet_variable->packet.fields(i);
		r = reflectJsFieldsToMessagePacketField(r, context, options_cache, f);
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
		std::string field_packet = reflectJsFieldsToMessagePacket(r.substr(fld.offset(), fld.size()), context, options_cache, &packet_var);
		r = r.substr(0, fld.offset()) + field_packet + r.substr(fld.offset() + fld.size(), r.size() - (fld.offset() + fld.size()));
	}
	return r;
}
