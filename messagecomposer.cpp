#include <sstream>
#include <stdlib.h>

#include "messagecomposer.h"
#include "errorcodes.h"
#include "bin2ascii.h"

static double parse_double(const std::string &v)
{
	return atof(v.c_str());
}

static float parse_float(const std::string &v)
{
	return atof(v.c_str());
}

static int64_t parse_int64_t(const std::string &v)
{
	return strtoll(v.c_str(), NULL, 10);
}

static uint64_t parse_uint64_t(const std::string &v)
{
	return strtoull(v.c_str(), NULL, 10);
}

static int32_t parse_int32_t(const std::string &v)
{
	return strtol(v.c_str(), NULL, 10);
}

static uint32_t parse_uint32_t(const std::string &v)
{
	return strtoul(v.c_str(), NULL, 10);
}

static double parse_bool(const std::string &v)
{
	return !v.empty() && v[0] != '0' && v[0] !='f' && v[0] !='F';
}

MessageComposer::MessageComposer()
	: env(NULL), onComposeField(NULL), onMessageBegin(NULL), optionCache(NULL)
{
}

MessageComposer::~MessageComposer() {
}

MessageComposer::MessageComposer
(
		void *environment,
		const Pkt2OptionsCache *options,
		google::protobuf::Message *message,
		oncompose_field fieldCallback,
        on_message_begin callback_message_begin,
        on_message_end callback_message_end

)
	: env(environment), onComposeField(fieldCallback), 
        onMessageBegin(callback_message_begin), onMessageEnd(callback_message_end), optionCache(options)
{
	compose(NULL, message, false, 0);
}

void MessageComposer::setCallbacks
(
	oncompose_field oncomposefield,
    on_message_begin onmessagebegin,
    on_message_end onmessageend
)
{
	this->onComposeField = oncomposefield;
	this->onMessageBegin = onmessagebegin;
    this->onMessageEnd = onmessageend;
}

#define COMPOSE_TYPE(NATIVE_TYPE, PROTO_TYPE) \
{ \
    if (repeated) \
    { \
        int i = 0; \
        while (onComposeField(env, message_descriptor, t, field->number(), true, i, value)) \
        { \
            ref->Add ## PROTO_TYPE(message, field, parse_ ## NATIVE_TYPE(value)); \
            i++; \
        } \
    } \
    else \
    { \
        onComposeField(env, message_descriptor, t, field->number(), false, 0, value); \
        ref->Set ## PROTO_TYPE(message, field, parse_ ## NATIVE_TYPE(value)); \
    } \
}

/**
 * @brief Compose field
 * @param message message
 * @param field field
 * @return 0- success
 */
int MessageComposer::composeField
(
    google::protobuf::Message *message,
	const google::protobuf::FieldDescriptor *field
)
{
    const google::protobuf::Descriptor *message_descriptor = message->GetDescriptor();
    if (!field)
        return ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR;
    google::protobuf::FieldDescriptor::CppType t = field->cpp_type();

    const google::protobuf::Reflection *ref = message->GetReflection();
    const bool repeated = field->is_repeated();

    size_t array_size = 0;
    if (repeated)
        array_size = ref->FieldSize(*message, field);

    int index = 0;

    if (optionCache)
    {
    	// check is it index
    	index = optionCache->getIndex(message_descriptor->full_name(), field->name());
    }

    std::string value;
    switch (t)
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        	COMPOSE_TYPE(double, Double)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        	COMPOSE_TYPE(float, Float)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        	COMPOSE_TYPE(int64_t, Int64)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        	COMPOSE_TYPE(uint64_t, UInt64)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        	COMPOSE_TYPE(int32_t, Int32)
			break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        	COMPOSE_TYPE(uint32_t, UInt32)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        	COMPOSE_TYPE(bool, Bool)
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            bool is_binary = field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES;
            if (repeated)
            {
                int i = 0;
                while (onComposeField(env, message_descriptor, t, field->number(), true, i, value))
                {
                    if (is_binary)
                        value = b64_encode(value);
                    ref->AddString(message, field, value);
                    i++;
                }
            }
            else
            {
                onComposeField(env, message_descriptor, t, field->number(), false, 0, value);
                if (is_binary)
                    value = b64_encode(value);
                ref->SetString(message, field, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
            if (repeated)
            {
                int i = 0;
                google::protobuf::Message *mf = ref->AddMessage(message, field);
                while (compose(field, mf, true, i))
				{
                	i++;
				}
            }
            else
            {
                google::protobuf::Message *mf = ref->MutableMessage(message, field);
                compose(field, mf, false, 0);
            }
            break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        	{
                if (repeated)
                {
                    int i = 0;
                    while (onComposeField(env, message_descriptor, t, field->number(), true, i, value))
                    {
                        ref->AddEnumValue(message, field, parse_int32_t(value));
                        i++;
                    }
                }
                else
                {
                    onComposeField(env, message_descriptor, t, field->number(), false, 0, value);
                    ref->SetEnumValue(message, field, parse_int32_t(value));
                }
        	}
            break;
        default:
            break;
    }
	return ERR_OK;	
}

bool MessageComposer::compose
(
    const google::protobuf::FieldDescriptor *field,
	google::protobuf::Message *message,
	bool repeated,
	int index
)
{
    onMessageBegin(env, field, message, repeated, index);
    const google::protobuf::Descriptor *message_descriptor = message->GetDescriptor();
    if (!message_descriptor)
        return ERRCODE_DECOMPOSE_NO_MESSAGE_DESCRIPTOR;
    size_t count = message_descriptor->field_count();
    for (size_t i = 0; i != count; ++i)
    {
        const google::protobuf::FieldDescriptor *field = message_descriptor->field(i);
        if (composeField(message, field))
        {
            return ERRCODE_DECOMPOSE_FATAL;
        }
    }
    onMessageEnd(env, message, repeated, index);
    return repeated;
}
