#include <string>
#include <iostream>
#include "utilprotobuf.h"
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
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/compiler/importer.h>

#include <glog/logging.h>

#include "error-printer.h"

const std::string fileNameSuffixProto = (".proto");

std::map<std::string, const google::protobuf::Descriptor*> internalMessages;

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool parseProtoFile2
(
		const char *fn,
		std::map<std::string, const google::protobuf::Descriptor*> &messages
)
{
	FILE *f = fopen(fn, "r");
	if (f == NULL)
		return false;

	vector<const FileDescriptor*> parsed_files;

	// Set up the source tree.
	DiskSourceTree source_tree;

	MFErrorPrinter mf_error_printer;
	// Allocate the Importer.
	source_tree.MapPath("", "/home/andrei/src/pkt2");

	Importer importer(&source_tree, &mf_error_printer);

	// Import the file.
	const FileDescriptor* parsed_file1 = importer.Import("proto/pkt2.proto");
	// importer.AddUnusedImportTrackFile("proto/pkt2.proto");
	// importer.AddUnusedImportTrackFile(fn);

	const FileDescriptor* parsed_file = importer.Import(fn);
	importer.ClearUnusedImportTrackFiles();
	if (parsed_file == NULL)
	{
		fclose(f);
		return false;
	}
	parsed_files.push_back(parsed_file);

	FileInputStream proto_stream(fileno(f));
	Tokenizer input_proto(&proto_stream, NULL);

	FileDescriptorProto file_desc_proto;
	Parser parser;
	parser.RecordErrorsTo(&mf_error_printer);

	bool ok = parser.Parse(&input_proto, &file_desc_proto);
	if (!ok)
	{

	}

	google::protobuf::DescriptorPool pool;
	pool.EnforceWeakDependencies(true);

	const google::protobuf::FileDescriptor* file_desc = pool.BuildFile(file_desc_proto);
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
		messages[md->name()] = md;
	}


	fclose(f);

	return true;
}

namespace utilProto
{

	/**
	 * for nftw() use only
	 */
	bool parseProtoFile
	(
		const char *path
	)
	{
		LOG(INFO) << "parseProtoFile: " << path;
		return parseProtoFile2(path, internalMessages);
	}

	void debugProto
	(
		const std::map<std::string, const google::protobuf::Descriptor*> *messages
	)
	{
		for (auto it = messages->begin(); it != messages->end(); ++it)
		{
			std::cout << it->first << " => " << it->second->DebugString() << std::endl;
		}
	}

	int onProtoFile
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
					bool ok = utilProto::parseProtoFile(path);
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


	std::map<std::string, const google::protobuf::Descriptor*> *parseProtoFiles
	(
		const std::string &path
	)
	{
		iteratePath(path, onProtoFile);
		return &internalMessages;
	}

}
