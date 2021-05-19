#include "env-pkt2.h"

EnvPkt2::EnvPkt2(
	const std::string &proto_path,
	int verbosity
) {
	declarations = new ProtobufDeclarations(proto_path, verbosity);
	options_cache = new Pkt2OptionsCache(declarations);
	packet2Message = new Packet2Message(declarations, options_cache, verbosity);
}

EnvPkt2::~EnvPkt2() {
	if (packet2Message) {
		delete packet2Message;
	}
	if (options_cache) {
		delete options_cache;
	}
	if (declarations) {
		delete declarations;
	}
}
