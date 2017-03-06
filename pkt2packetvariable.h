/*
 * pkt2packetvariable.h
 *
 */

#ifndef PKT2PACKETVARIABLE_H_
#define PKT2PACKETVARIABLE_H_

#include <vector>
#include <google/protobuf/message.h>

#include "pkt2.pb.h"
#include "protobuf-declarations.h"
#include "utilprotobuf.h"


class Pkt2PacketVariable {
public:
	Pkt2PacketVariable();
	Pkt2PacketVariable
	(
		ProtobufDeclarations *pd,
		MessageTypeNAddress *messageTypeNAddress
	);
	
	virtual ~Pkt2PacketVariable();
	
	int status;
	pkt2::Packet packet;
	std::vector<pkt2::Variable> variables; 
};

#endif /* PKT2PACKETVARIABLE_H_ */
