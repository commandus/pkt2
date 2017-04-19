/*
 * pkt2packetvariable.h
 *
 */

#ifndef PKT2PACKETVARIABLE_H_
#define PKT2PACKETVARIABLE_H_

#include <string>
#include <vector>
#include <map>
#include <google/protobuf/message.h>

#include "protobuf-declarations.h"
#include "pkt2.pb.h"

/**
 * @brief Keep pkt2::Variable with name of the Protobuf field name intact.
 * Variable is metadata describes Protobuf message (output)
 *
 */
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
 * @brief Keep message options:
 * 	- pkt2::Packet - input metadata
 * 	- pkt2::Variables - output metadata
 * 	- helper indexes
 *
 * @see pkt2optionscache.cpp
 */
class Pkt2PacketVariable {
private:
	// keep field number to the vector index
	std::map<int, int> fieldNumbers;
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
	/// Input metadata
	pkt2::Packet packet;
	/// Minimum packet size
	size_t packet_size;
	std::vector<FieldNameVariable> fieldname_variables;
	/// keep variables vector index having index in order of 1, 2
	/// first is identifier (or hash) of the message
	std::vector<uint64_t> keyIndexes;
	const FieldNameVariable* getVariableByFieldNumber(
        int field_number) const;
	bool validTags(const std::string &packet);
	/// debugging information
	std::string toString();
};

#endif /* PKT2PACKETVARIABLE_H_ */
