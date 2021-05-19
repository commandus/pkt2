#ifndef ENV_PKT2_H_
#define ENV_PKT2_H_

/**
 * Helper class
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexpansion-to-defined"
#include "packet2message.h"
#pragma clang diagnostic pop


class EnvPkt2 {
public:
	ProtobufDeclarations *declarations;
	Pkt2OptionsCache *options_cache;
	Packet2Message *packet2Message;
	explicit EnvPkt2(
		const std::string &proto_path,
		int verbosity
	);

	~EnvPkt2();
};

#endif
