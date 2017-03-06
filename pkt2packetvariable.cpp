/*
 * pkt2packetvariable.cpp
 *
 */
#include <glog/logging.h>

#include "pkt2.pb.h"
#include "pkt2packetvariable.h"

#include "errorcodes.h"

using namespace google::protobuf;

Pkt2PacketVariable::Pkt2PacketVariable()
	: status(ERR_OK) 
{

}

Pkt2PacketVariable::Pkt2PacketVariable
(
	ProtobufDeclarations *pd,
	MessageTypeNAddress *messageTypeNAddress
)
{
	status = ERR_OK;

	// Each message
	const google::protobuf::Descriptor* md = pd->getMessageDescriptor(messageTypeNAddress->message_type);
	if (!md)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << messageTypeNAddress->message_type;
		status = ERRCODE_MESSAGE_TYPE_NOT_FOUND;
	}

	const google::protobuf::MessageOptions options = md->options();
	if (options.HasExtension(pkt2::packet))
	{
		packet = options.GetExtension(pkt2::packet);
	}

	for (int f = 0; f < md->field_count(); f++)
	{
		const google::protobuf::FieldOptions foptions = md->field(f)->options();
		std::string out;
		pkt2::Variable variable = foptions.GetExtension(pkt2::variable);
		variables.push_back(variable);
	}

}

Pkt2PacketVariable::~Pkt2PacketVariable() {
}

