/*
 * pkt2packetvariable.h
 *
 */

#ifndef PKT2PACKETVARIABLE_H_
#define PKT2PACKETVARIABLE_H_

#include <string>
#include <vector>
#include <google/protobuf/message.h>

#include "protobuf-declarations.h"
#include "pkt2.pb.h"

class FieldNameVariable {
public:
	FieldNameVariable
	(
		const std::string &fieldname,
		pkt2::Variable variable
	)
		: field_name(fieldname), var(variable)
	{};
	std::string field_name;
	pkt2::Variable var;
};

/**
 * Keep message options: packet & variables and indexes
 */
class Pkt2PacketVariable {
public:
	Pkt2PacketVariable();
	Pkt2PacketVariable
	(
		const std::string &message_type,
		ProtobufDeclarations *pd
	);
	
	virtual ~Pkt2PacketVariable();
	
	int status;
	std::string message_type;
	pkt2::Packet packet;
	std::vector<FieldNameVariable> fieldname_variables;
	/// keep variables vector index having index in order of 1, 2
	/// first is identifier (or hash) of the message
	std::vector<uint64_t> keyIndexes;
};

#endif /* PKT2PACKETVARIABLE_H_ */
