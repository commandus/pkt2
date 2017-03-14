/*
 * pkt2packetvariable.cpp
 *
 */
#include <functional>
#include <glog/logging.h>

#include "pkt2.pb.h"
#include "pkt2packetvariable.h"

#include "errorcodes.h"

using namespace google::protobuf;

Pkt2PacketVariable::Pkt2PacketVariable()
	: status(ERR_OK), message_type("")
{

}

Pkt2PacketVariable::Pkt2PacketVariable
(
	const std::string &message_type,
	ProtobufDeclarations *pd
)
{
	status = ERR_OK;

	// get packet name
	this->message_type = message_type;
	// get message descriptor
	const google::protobuf::Descriptor* md = pd->getMessageDescriptor(message_type);
	if (!md)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << message_type;
		status = ERRCODE_MESSAGE_TYPE_NOT_FOUND;
	}

	// get packet
	const google::protobuf::MessageOptions options = md->options();
	if (options.HasExtension(pkt2::packet))
	{
		packet = options.GetExtension(pkt2::packet);
		// set message identifier if not assigned in the proto file
		if (!packet.id())
		{
			std::hash<std::string> h;
			size_t id = h(packet.name());
			packet.set_id(id);
		}
	}

	// temporarily used for sorting
	std::map<int, int> indexVariable;

	// get fields
	for (int f = 0; f < md->field_count(); f++)
	{
		const google::protobuf::FieldOptions foptions = md->field(f)->options();
		std::string out;
		pkt2::Variable variable = foptions.GetExtension(pkt2::variable);
		variables.push_back(variable);


		// prepare index
		int index = variable.index();
		if (index)
			indexVariable[index] = f;
		// calculate hash of the name as identifier if identifier is 0
	}

	// create index
	// first is message identifier itself
	keyIndexes.push_back(packet.id());
	for (int i = 1; i <= indexVariable.size(); i++)
		keyIndexes.push_back(indexVariable[i]);
#ifdef DEBUG
	std::stringstream ss;
	for (uint64_t idx : keyIndexes)
	{
		ss << idx << " ";
	}
	LOG(ERROR) << message_type << " " << " indexes: " << ss.str();
#endif
}

Pkt2PacketVariable::~Pkt2PacketVariable() {
}

