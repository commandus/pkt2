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

/**
 * Keep all parsed messages options as map of message name and Pkt2PacketVariable pairs
 * @see Pkt2PacketVariable
 */
class Pkt2OptionsCache {
private:
	int size_of(enum pkt2::OutputType t);
public:
	Pkt2OptionsCache();
	Pkt2OptionsCache(ProtobufDeclarations *protobuf_declarations);
	void addDeclarations(ProtobufDeclarations *protobuf_declarations);
	
	virtual ~Pkt2OptionsCache();
	
	std::map<std::string, Pkt2PacketVariable> pkt2packet_variable;

	const Pkt2PacketVariable &getPacketVariable
	(
			const std::string &message_type,
			bool *found
	);

	/**
	 * Check if field have index
	 * @param message_type message name
	 * @param field_type field name
	 * @return 0- no index, 1,2.. index
	 */
	int getIndex
	(
		const std::string &message_type,
		const std::string &field_type
	);
	/**
	 * Copy key from the message
	 * @param messageType message name
	 * @param buffer retval
	 * @param max_size max buffer size
	 * @param message get key values from
	 * @return
	 */
	size_t getKey
	(
		const std::string &messageType,
		void *buffer,
		size_t max_size,
		const google::protobuf::Message *message
	);

	/**
	 * Return message identifier (or message name hash if id is not assigned)
	 * @param messageType
	 * @return
	 */
	uint64_t getMessageId
	(
		const std::string &messageType
	);

};

#endif /* PKT2OPTIONSCACHE_H_ */
