#ifndef UTILPROTOBUF_H
#define UTILPROTOBUF_H

#include <sys/socket.h>
#include <string>
#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "protobuf-declarations.h"

#define MAX_PROTO_TOTAL_BYTES_LIMIT 	512 * 1024 * 1024

#define SOCKADDR_SIZE sizeof(struct sockaddr)	// 16 bytes

class MessageTypeNAddress
{
public:
	std::string message_type;
    struct sockaddr socket_address_src;
    struct sockaddr socket_address_dst;
    uint32_t message_size;

	MessageTypeNAddress();
	MessageTypeNAddress(const std::string &messagetype);

	size_t getKey(
			void *buffer,
			size_t max_size,
			const google::protobuf::Message *message
	);
};

namespace utilProto
{

/**
 * @brief Parse proto file
 * @param path
 * @param filename
 * @param messages
 * @param error_output
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
 * @param messages
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
 * @param path
 * @param messages
 * @param error_output std::ostream
 * @return successfully parsed files count
 */
size_t parseProtoPath
(
	const std::string &path,
	std::map<std::string, const google::protobuf::Descriptor*> &messages,
	std::ostream *error_output
);

void debugProto
(
	const std::map<std::string,
	const google::protobuf::Descriptor*> *messages
);
}

/**
 * @brief Write Message type string, size of message and message itself
 *
 * @param messageTypeName
 * @param message
 * @param rawOutput
 * @return
 */
int writeDelimitedMessage
(
	const MessageTypeNAddress *messageTypeNAddress,
    const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput
);

/**
 * @brief Write Message type string, size of message and message itself to the string
 * @param messageTypeName
 * @param message
 * @return
 */
std::string stringDelimitedMessage
(
	const MessageTypeNAddress *messageTypeNAddress,
    const google::protobuf::MessageLite& message
);

/**
 * Read delimited message from the coded stream
 * @param strm
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		google::protobuf::io::CodedInputStream *strm,
		MessageTypeNAddress *messageTypeNAddress
);

/**
 * Read delimited message from the input stream
 * @param strm
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		std::istream *strm,
		MessageTypeNAddress *messageTypeNAddress
);

/**
 * Read delimited message from the string
 * @param value
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		std::string &buffer,
		MessageTypeNAddress *messageTypeNAddress
);

/**
 * Read delimited message from the buffer
 * @param buffer
 * @param size
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		void *buffer,
		int size,
		MessageTypeNAddress *messageTypeNAddress
);

#endif /* UTILPROTOBUF_H */
