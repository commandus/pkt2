/*
 * packet2message.cpp
 *
 *  Created on: Mar 24, 2017
 *      Author: andrei
 */

#include <glog/logging.h>
#include "packet2message.h"
#include "pkt2packetvariable.h"
#include "errorcodes.h"

Packet2Message::Packet2Message(
		const std::string &protopath,
		int verbose
)
	: verbosity(verbose)
{
	declarations = new ProtobufDeclarations(protopath, verbosity);
	options_cache = new Pkt2OptionsCache(declarations);
}

Packet2Message::~Packet2Message() {
	delete options_cache;
	delete declarations;
}

/**
 *
 * @param packet
 * @param force_message packet.message or "" (no force)
 * @return
 */google::protobuf::Message *Packet2Message::parse
(
	const std::string &packet,
	const std::string &force_message
)
{
	 if (!force_message.empty())
	 {
		 bool found;
		 const Pkt2PacketVariable &pv = options_cache->getPacketVariable(force_message, &found);
		 if (found)
		 {
			 if (verbosity > 0)
				 LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << force_message;
			 return NULL;
		 }
	 }
	return NULL;	
}
