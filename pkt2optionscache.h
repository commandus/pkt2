/*
 * pkt2optionscache.h
 *
 *  Created on: Mar 7, 2017
 *      Author: andrei
 */

#ifndef PKT2OPTIONSCACHE_H_
#define PKT2OPTIONSCACHE_H_

#include <string>
#include <map>
#include "pkt2.pb.h"
#include "pkt2packetvariable.h"

class Pkt2OptionsCache {
private:
	int size_of(enum pkt2::OutputType t);
public:
	Pkt2OptionsCache();
	Pkt2OptionsCache(ProtobufDeclarations *protobuf_declarations);
	void addDeclarations(ProtobufDeclarations *protobuf_declarations);
	
	virtual ~Pkt2OptionsCache();
	
	std::map<std::string, Pkt2PacketVariable> pkt2;

	size_t getKey(
			const std::string &messageType,
			void *buffer,
			size_t max_size,
			const google::protobuf::Message *message
	);
};

#endif /* PKT2OPTIONSCACHE_H_ */
