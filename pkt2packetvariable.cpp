/*
 * pkt2packetvariable.cpp
 *
 */
#include <functional>
#include <iomanip>
#include <glog/logging.h>

#include "pkt2.pb.h"
#include "pkt2packetvariable.h"

#include "errorcodes.h"
#include "utilprotobuf.h"


using namespace google::protobuf;

Pkt2PacketVariable::Pkt2PacketVariable()
	: status(ERR_OK), message_type(""), packet_size(0)
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
		fieldname_variables.push_back(FieldNameVariable(md->field(f)->name(), variable));
		// keep field number to the vector index
		fieldNumbers.insert(pair<int, int>(md->field(f)->number(), f));

		// prepare index
		int index = variable.index();
		if (index)
			indexVariable[index] = f;
		// calculate hash of the name as identifier if identifier is 0
	}

	packet_size = getPacketSize(packet);

	// create index
	// first is message identifier itself
	keyIndexes.push_back(packet.id());
	for (int i = 1; i <= indexVariable.size(); i++)
		keyIndexes.push_back(indexVariable[i]);
}

/**
 * @brief debugging information
 * @return
 */
std::string Pkt2PacketVariable::toString()
{
	std::stringstream ss;
	ss << "Packet: " << packet.id() << " size: " << packet_size << std::endl;
	ss << "Variables: "  << std::endl;
	for (std::vector<FieldNameVariable>::const_iterator it(fieldname_variables.begin());
			it != fieldname_variables.end(); ++it)
	{
		ss << std::setw(16) << it->field_name << ": " << it->var.get() <<  ", format: " << it->var.get() << std::endl;
	}

	return ss.str();
}

Pkt2PacketVariable::~Pkt2PacketVariable() {
}

const FieldNameVariable* Pkt2PacketVariable::getVariableByFieldNumber
(
	int field_number
) const
{
	auto f = fieldNumbers.find(field_number);
	if (f == fieldNumbers.end())
		return NULL;
	return &fieldname_variables[f->second];
}

/**
  * @brief Check is all tags found
  * @param packet packet to parse
  * @return true if tags found
  **/
bool Pkt2PacketVariable::validTags
(
	const std::string &data
) const
{
	// TODO if (data.size() < packet_size)
	if (data.size() != packet_size)
	{
		return false;
	}
	for (int i = 0; i < packet.fields_size(); i++)
	{
		uint64_t t = packet.fields(i).tag();
		if (t)
		{
			// special case
			if (t == 0xFFFFFFFFFFFFFFFF)
				t = 0;
			uint64_t f = extractFieldUInt(data, packet.fields(i));
			if (f != t)
			{
				LOG(ERROR) << "tag " << packet.fields(i).name() << " not found . Request: " << t << ", value: " << f;
				return false;
			}
		}
	}
	return true;
}
