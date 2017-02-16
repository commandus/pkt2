/*
 * protobuf-declarations.cpp
 *
 *  Created on: Feb 15, 2017
 *      Author: andrei
 */

#include "protobuf-declarations.h"

#include <string>
#include <iostream>
#include <vector>

#include "utilfile.h"

#ifdef _MSC_VER
#include <windows.h>
#define PATH_DELIMITER "\\"
#else
#include <ftw.h>
#define PATH_DELIMITER "/"
#endif

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/parser.h>

#include <glog/logging.h>

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

const std::string fileNameSuffixProto = (".proto");

ProtobufDeclarations::ProtobufDeclarations(const std::string &path) 
{
	paths.push_back(path);

	// Allocate the Importer.
	for (const std::string &path : paths)
	{
		source_tree.MapPath("", path);
	}
	importer = new Importer(&source_tree, &mf_error_printer);
}

ProtobufDeclarations::~ProtobufDeclarations() {
	if (importer)
		delete importer;
}

std::map<std::string, const google::protobuf::Descriptor*> *ProtobufDeclarations::getMessages()
{
	return &internalMessages;
}

/**
 * 	const google::protobuf::DescriptorPool* pool,
 *
 */
bool ProtobufDeclarations::decode
(
	const std::string &message_name,
	google::protobuf::io::IstreamInputStream *stream,
	uint32_t size
)
{
	// Look up the type.
	const Descriptor* type = importer->pool()->FindMessageTypeByName(message_name);
	if (type == NULL)
		return false;

	DynamicMessageFactory dynamic_factory(importer->pool());
	google::protobuf::scoped_ptr<Message> message(dynamic_factory.GetPrototype(type)->New());

	// Input is binary.
	if (!message->ParsePartialFromZeroCopyStream(stream))
		return false;

	io::FileOutputStream out(STDOUT_FILENO);
	TextFormat::Print(*message, &out);
	return true;
}

bool ProtobufDeclarations::parseProtoFile
(
	const char *fn
)
{
	// Import the file.
	importer->AddUnusedImportTrackFile(fn);
	const FileDescriptor* parsed_file = importer->Import(fn);
	importer->ClearUnusedImportTrackFiles();
	if (parsed_file == NULL)
	{
		LOG(ERROR) << "Cannot import proto file " << fn;
		return false;
	}
	parsed_files.push_back(parsed_file);

	FILE *f = fopen(fn, "r");
	if (f == NULL)
	{
		std::string fn1 = concatPath(fn, 0);
		f = fopen(fn1.c_str(), "r");
		if (f == NULL)
			return false;
	}
	FileInputStream proto_stream(fileno(f));
	Tokenizer input_proto(&proto_stream, NULL);

	FileDescriptorProto file_desc_proto;
	Parser parser;
	parser.RecordErrorsTo(&mf_error_printer);

	bool ok = parser.Parse(&input_proto, &file_desc_proto);
	if (!ok)
	{
		LOG(ERROR) << "Cannot parse proto file " << fn;
		return false;
	}

	// get "top level" file and print out messages
	const FileDescriptor *file_desc = importer->pool()->FindFileByName(fn);

	if (file_desc == NULL)
	{
		LOG(ERROR) << "Cannot get file descriptor from file descriptor proto";
		fclose(f);
		return false;
	}

	int count = file_desc->message_type_count();
	for (int m = 0; m < count; m++)
	{
		const google::protobuf::Descriptor* md = file_desc->message_type(m);
		LOG(ERROR) << "message: " << md->DebugString();
		internalMessages[md->name()] = md;
	}

	fclose(f);

	return true;
}
/**
 * Add path from paths at specified index to the file name
 */
std::string ProtobufDeclarations::concatPath
(
	const std::string &fn, int index
)
{
	return paths[index] + "/" + fn;
}



int ProtobufDeclarations::onProtoFile
(
	const char *path,
	const struct stat *ptr,
	int flag,
	struct FTW *ftwbuf
)
{
	switch (flag)
	{
	case FTW_D:
	case FTW_DP:
	case FTW_DNR:
		return 0;
	case FTW_F:
	case FTW_SL:
		{
			if (strcasestr(path, fileNameSuffixProto.c_str()) >= 0)
			{
				// ".proto"
				std::cerr << path << ".. ";
				bool ok = parseProtoFile(path);
				if (ok)
					std::cerr << "ok";
				else
					std::cerr << "failed";
				std::cerr << std::endl;
			}
		}
		break;

	default:
		return 0;
	}
	return 0;
}

void ProtobufDeclarations::debug
(
	const std::map<std::string, const google::protobuf::Descriptor*> *messages
)
{
	for (auto it = messages->begin(); it != messages->end(); ++it)
	{
		std::cout << it->first << " => " << it->second->DebugString() << std::endl;
	}
}
