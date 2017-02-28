#ifndef UTILPROTOBUF_H
#define UTILPROTOBUF_H

#include <string>
#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

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

#endif /* UTILPROTOBUF_H */
