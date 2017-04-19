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
 * @brief Keep all parsed messages options as map of message name and Pkt2PacketVariable pairs
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
    
	/**
     * packet name.message type = Packet Variable
     */
	std::map<std::string, Pkt2PacketVariable> pkt2packet_variable;

    /**
    * @brief Find out PacketVariable by full message type name (Protobuf_packet_name.message_type)
    * @param message Protobuf full type name (with packet name)
    * @param found return value true- found
    * @return found PacketVariable
    */
	const Pkt2PacketVariable &getPacketVariable
	(
			const std::string &message_type,
			bool *found
	) const;

    /**
    * @brief Find out first found PacketVariable by size and tag
    * @param packet packet data
    * @param found return value true- found
    * @return found PacketVariable
    */
	const Pkt2PacketVariable &find1
	(
			const std::string &packet,
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
	) const;
	
	/**
	 * @brief Copy key from the message
	 * @param messageType message name
	 * @param buffer retval
	 * @param max_size max buffer size
	 * @param message get key values from
	 * @return key
	 */
	size_t getKey
	(
		const std::string &messageType,
		void *buffer,
		size_t max_size,
		const google::protobuf::Message *message
	);

	/**
	 * @brief Return message identifier (or message name hash if id is not assigned)
	 * @param messageType full messaget type name
	 * @return unique message identifier
	 */
	uint64_t getMessageId
	(
		const std::string &messageType
	);

	/**
	 * @brief Keep "parent" decalrations. 
	 * Warning: referred object can be deleted!
	 */
	ProtobufDeclarations *protobuf_decrarations;
};

#endif /* PKT2OPTIONSCACHE_H_ */
