/*
 * packet2message.h
 *
 */

#ifndef PACKET2MESSAGE_H_
#define PACKET2MESSAGE_H_

#include <deque>
#include <google/protobuf/message.h>

#include "javascript-context.h"

#include "pkt2optionscache.h"
#include "protobuf-declarations.h"

/**
 * @brief Packet container with initialize javascript environment ready to parse.
 * The main reason why packet + javascript environment are separated is
 * re-use by different parsers.
 */
class PacketParseEnvironment
{
private:
    std::deque<std::string> fieldnames;
public:
	const std::string &packet;
    const Pkt2OptionsCache *options_cache;
    const Pkt2PacketVariable *packet_root_variable;
	JavascriptContext *context;
	const std::map<std::string, std::string> *tableAliases;
	const std::map<std::string, std::string> *fieldAliases;
	const std::map<std::string, std::string> *properties;

	PacketParseEnvironment
	(
		struct sockaddr *socket_address_src,
		struct sockaddr *socket_address_dst,
		const std::string &packet,
		Pkt2OptionsCache *options_cache,
		const std::string &force_message,
		const std::map<std::string, std::string> *tableAliases = NULL,
		const std::map<std::string, std::string> *fieldAliases = NULL,
		const std::map<std::string, std::string> *properties = NULL
	);
	~PacketParseEnvironment();
    std::string getFullFieldName();
    void pushFieldName(const std::string &field_name);
    void popFieldName();
};

/**
 * @brief Packet parser to corresponding message
 */
class Packet2Message {
private:
	int verbosity;
	const ProtobufDeclarations *declarations;
	const Pkt2OptionsCache *options_cache;
public:
    Packet2Message(
        const ProtobufDeclarations* protobufdeclarations, 
        const Pkt2OptionsCache* optionscache, 
        int verb
    );
	virtual ~Packet2Message();

	/**
	 * @brief Parse packet
	 * @param socket_address_src can be NULL
	 * @param socket_address_dst can be NULL
	 * @param packet data
	 * @param size data size
	 * @param force_message packet.message or "" (no force)
	 * @return Protobuf message
	 */
	google::protobuf::Message *parsePacket
	(
		PacketParseEnvironment *env,
		const std::map<std::string, std::string> *tableAliases = NULL,
		const std::map<std::string, std::string> *fieldAliases = NULL,
		const std::map<std::string, std::string> *properties = NULL
	);

	/**
	 * @brief Get Protobuf message by name
	 * @param message packet.message or "" (no force)
	 * @return Protobuf message
	 */
	google::protobuf::Message *getMessageByName
	(
		const std::string &messageType
	);

};

#endif /* PACKET2MESSAGE_H_ */
