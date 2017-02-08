#ifndef UTILPROTOBUF_H
#define UTILPROTOBUF_H

#include <string>
#include <map>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

namespace utilProto
{
	/**
	 * for nftw() use only
	 */
	/// Each protobuf3 file must have .proto file name suffix
	std::map<std::string, const google::protobuf::Descriptor*> *parseProtoFiles
	(
		const std::string &path
	);

	void debugProto
	(
		const std::map<std::string,
		const google::protobuf::Descriptor*> *messages
	);
}

#endif /* UTILPROTOBUF_H */
