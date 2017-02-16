/*
 * protobuf-declarations.h
 */

#ifndef PROTOBUF_DECLARATIONS_H_
#define PROTOBUF_DECLARATIONS_H_

#include <map>
#include <vector>
#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/compiler/importer.h>
#include "error-printer.h"

class ProtobufDeclarations {
private:
	std::vector<std::string> paths;
	std::map<std::string, const google::protobuf::Descriptor*> internalMessages;
	std::vector<const google::protobuf::FileDescriptor*> parsed_files;

	// Set up the source tree.
	google::protobuf::compiler::DiskSourceTree source_tree;

	MFErrorPrinter mf_error_printer;

	google::protobuf::compiler::Importer *importer;

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
public:
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

	bool decode
	(
		const std::string &message_name,
		google::protobuf::io::IstreamInputStream *stream,
		uint32_t size
	);

	void debug
	(
		const std::map<std::string, const google::protobuf::Descriptor*> *messages
	);
};

#endif
