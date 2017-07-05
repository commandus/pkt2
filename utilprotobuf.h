#ifndef UTILPROTOBUF_H
#define UTILPROTOBUF_H

#include <sys/socket.h>
#include <string>
#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "pkt2optionscache.h"

#define MAX_PROTO_TOTAL_BYTES_LIMIT 	512 * 1024 * 1024

#define SOCKADDR_SIZE sizeof(struct sockaddr)	// 16 bytes

/**
 * @brief Keep Protobuf message full type name (packet.name), source and destination IP addresses, size of serialize message
 * 
 **/
class MessageTypeNAddress
{
public:
	/// Protobuf message full type name (packet.name)
	std::string message_type;
	/// source IP address
    struct sockaddr socket_address_src;
	/// destination IP address
    struct sockaddr socket_address_dst;
	/// message size
    uint32_t message_size;

	MessageTypeNAddress();
	MessageTypeNAddress(const std::string &messagetype);
};

namespace utilProto
{

	/**
	* @brief Parse proto file(s) in the file or directory
	* @param filename file or directory containing .proto files
	* @param path include path 
	* @param messages parsed message descriptors stored in the map. Make key is full Protobuf message name (packet.message)
	* @param error_output stream to receiev error reports
	* @return
	*/
	bool parseProtoFile
	(
		const char *filename,
		const std::string &path,
		std::map<std::string, const google::protobuf::Descriptor*> &messages,
		std::ostream *error_output
	);

	/**
	* @brief Each protobuf3 file must have .proto file name suffix
	* @param path	include path
	* @param protoFiles vector of file path name strings
	* @param messages parsed message descriptors stored in the map. Make key is full Protobuf message name (packet.message)
	* @param error_output std::ostream
	* @return successfully parsed files count
	*/
	size_t parseProtoFiles
	(
		const std::string &path,
		const std::vector<std::string> &protoFiles,
		std::map<std::string, const google::protobuf::Descriptor*> &messages,
		std::ostream *error_output
	);

	/**
	* @brief Each protobuf3 file must have .proto file name suffix
	* @param path include path
	* @param messages parsed message descriptors stored in the map. Make key is full Protobuf message name (packet.message)
	* @param error_output std::ostream
	* @return successfully parsed files count
	*/
	size_t parseProtoPath
	(
		const std::string &path,
		std::map<std::string, const google::protobuf::Descriptor*> &messages,
		std::ostream *error_output
	);

	/**
	 * @brief print out to the stdout each message descriptor calling ->DebugString()
	 * @param messages parsed message descriptors stored in the map. Make key is full Protobuf message name (packet.message)
	 */
	void debugProto
	(
		const std::map<std::string, const google::protobuf::Descriptor*> *messages
	);
}

/**
 * Get field value from the packet
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 * @return copy of extracted area
 */
std::string extractField
(
		const std::string &packet,
		const pkt2::Field &field
);

/**
 * Get field value from the packet as double
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 * @return float value
 */
double extractFieldDouble
(
		const std::string &packet,
		const pkt2::Field &field
);

/**
 * @brief Get field value from the packet as 64 bit integer
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 * @return integer value
 */
uint64_t extractFieldUInt
(
		const std::string &packet,
		const pkt2::Field &field
);

/**
 * @brief Set field value in the packet from 64 bit integer
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 */
void setFieldUInt
(
	std::string &packet,
	const pkt2::Field &field,
	uint64_t value
);

/**
 * Return minimum size of the packet
 * @param packet binary data 
 * @return size
 */
size_t getPacketSize(const pkt2::Packet &packet);

/**
 * @brief Write Message type string, size of message and message itself
 *
 * @param messageTypeNAddress Protobuf message full name, size of serialized message, source and destination IP addresses
 * @param message Serialized Protobuf message 
 * @param rawOutput output stream
 * @return 0 - success
 */
int writeDelimitedMessage
(
	const MessageTypeNAddress *messageTypeNAddress,
    const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput
);

/**
 * @brief Write Message type string, size of message and message itself to the string
 * @param messageTypeName Protobuf message full name, size of serialized message, source and destination IP addresses
 * @param message Serialized Protobuf message 
 * @return as string 
 */
std::string stringDelimitedMessage
(
	const MessageTypeNAddress *messageTypeNAddress,
    const google::protobuf::MessageLite& message
);

/**
 * @brief Read delimited message from the coded stream
 * @param strm stream to read
 * @return message Protobuf message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		google::protobuf::io::CodedInputStream *strm,
		MessageTypeNAddress *messageTypeNAddress
);

/**
 * @brief Read delimited message from the input stream
 * @param strm stream to read
 * @return message Protobuf message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		std::istream *strm,
		MessageTypeNAddress *messageTypeNAddress
);

/**
 * @brief Read delimited message from the string
 * @param value stream as string
 * @return message Protobuf message

 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		std::string &buffer,
		MessageTypeNAddress *messageTypeNAddress
);

/**
 * @brief Read delimited message from the buffer
 * @param buffer buffer pointer
 * @param size size
 * @return message Protobuf message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		void *buffer,
		int size,
		MessageTypeNAddress *messageTypeNAddress
);

#endif /* UTILPROTOBUF_H */
