/*
 * protobuf-declarations.h
 * @see message2gateway-impl.cpp
 */

#ifndef PROTOBUF_DECLARATIONS_H_
#define PROTOBUF_DECLARATIONS_H_

#include <map>
#include <vector>
#include <string>

#include "google/protobuf/arena.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/compiler/importer.h>
#include "error-printer.h"

class ProtobufDeclarations {
private:
	// google::protobuf::Arena arena;
	std::vector<std::string> paths;
	std::map<std::string, const google::protobuf::Descriptor*> internalMessages;
	std::vector<const google::protobuf::FileDescriptor*> parsed_files;

	// Set up the source tree.
	google::protobuf::compiler::DiskSourceTree source_tree;

	MFErrorPrinter mf_error_printer;

	google::protobuf::compiler::Importer *importer;
	google::protobuf::DynamicMessageFactory *dynamic_factory;

	/**
	 * @brief Try add path from include paths at specified index to the file name and open
	 * @param fn file name to concatenate
	 * @param index path index
	 * @return
	 */
	FILE *openProto
	(
			const std::string &fn
	);

public:
	/**
	 * @brief After addPath() and parseProtoPath()
	 */
	ProtobufDeclarations();
	/**
	 * @brief Add virtual path "" and parse all files recursively
	 * @param path
	 */
	ProtobufDeclarations
	(
		const std::string &path
	);

	virtual ~ProtobufDeclarations();

	/**
	 * @brief add .proto include path
	 * @param virtual_path virtual path
	 * @param path include path
	 */
	void addPath
	(
			const std::string &virtual_path,
			const std::string &path
	);

	/**
	 * @brief add .proto include path
	 * @param path include path
	 */
	void addPath
	(
			const std::string &path
	);

	std::map<std::string, const google::protobuf::Descriptor*> *getMessages();
	bool parseProtoFile
	(
		const char *fn
	);

	/**
	 * Return protobuf messages
	 * @return count
	 */
	size_t getMessageCount();

	/**
	 * parse proto files
	 * @param protoFiles
	 * @return count successfully parsed files
	 */
	size_t parseProtoFiles
	(
		const std::vector<std::string> &protoFiles
	);

	/**
	 * @brief Each protobuf3 file must have .proto file name suffix
	 * @param path
	 * @return successfully parsed files count
	 */
	size_t parseProtoPath
	(
		const std::string &path
	);

	std::map<std::string, const google::protobuf::Descriptor*> *parseProtoDirectory
	(
		const std::string &path
	);

	google::protobuf::Message *decode
	(
		const std::string &message_name,
		google::protobuf::io::IstreamInputStream *stream
	);

	google::protobuf::Message *decode
	(
		const std::string &message_name,
		google::protobuf::io::CodedInputStream *stream
	);

	google::protobuf::Message *decode
	(
		google::protobuf::Arena *arena,
		const std::string &message_name,
		google::protobuf::io::CodedInputStream *stream
	);

	/**
	 * Print out messages to stdout
	 * @param messages
	 */
	void debugPrint
	(
		const std::map<std::string, const google::protobuf::Descriptor*> *messages
	);
};

#endif
