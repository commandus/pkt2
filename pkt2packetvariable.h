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
	std::vector<pkt2::Variable> variables;
	/// keep variables vector index having index in order of 1, 2
	std::vector<int> keyIndexes;
};

#endif /* PKT2PACKETVARIABLE_H_ */
