/*
 * protobuf-declarations.h
 */

#ifndef PROTOBUF_DECLARATIONS_H_
#define PROTOBUF_DECLARATIONS_H_

#include <map>
#include <vector>
#include <string>

#include <google/protobuf/descriptor.h>


class ProtobufDeclarations {
private:
	std::vector<std::string> paths;
	std::map<std::string, const google::protobuf::Descriptor*> internalMessages;

	int onProtoFile
	(
		const char *path,
		const struct stat *ptr,
		int flag,
		struct FTW *ftwbuf
	);

	/**
	 * Add path from paths at specified index to the file name
	 */
	std::string concatPath(const std::string &fn, int index);
	bool decode(
		const google::protobuf::DescriptorPool* pool,
		const std::string &message_name
	);

public:
	ProtobufDeclarations();
	
	ProtobufDeclarations(const std::string &path);
	
	virtual ~ProtobufDeclarations();
	
	std::map<std::string, const google::protobuf::Descriptor*> *getMessages();
	bool parseProtoFile
	(
		const char *fn
	);

	std::map<std::string, const google::protobuf::Descriptor*> *parseProtoDirectory
	(
		const std::string &path
	);
	
	void debug
	(
		const std::map<std::string, const google::protobuf::Descriptor*> *messages
	);
};

#endif
