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
#include "errorcodes.h"

/**
 * Proto file must have file name suffix ".proto"
 */
const std::string fileNameSuffixProto = (".proto");

// std::map<std::string, const google::protobuf::Descriptor*> internalMessages;

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool parseProtoFile2
(
		const char *fn,
		const std::string &path,
		std::map<std::string, const google::protobuf::Descriptor*> &messages,
		std::ostream *error_output
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
	source_tree.MapPath("", path);

	Importer importer(&source_tree, &mf_error_printer);

	// Import the file.
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
		if (error_output)
			*error_output << ERR_PARSE_PROTO << fn;
	}

	google::protobuf::DescriptorPool pool;
	pool.EnforceWeakDependencies(true);

	const google::protobuf::FileDescriptor* file_desc = pool.BuildFile(file_desc_proto);
	if (file_desc == NULL)
	{
		if (error_output)
			*error_output << ERR_PROTO_GET_DESCRIPTOR << fn;
		fclose(f);
		return false;
	}

	int count = file_desc->message_type_count();
	for (int m = 0; m < count; m++)
	{
		const google::protobuf::Descriptor* md = file_desc->message_type(m);
		if (error_output)
			*error_output << "message: " << md->DebugString();
		messages[md->name()] = md;
	}
	fclose(f);
	return true;
}

namespace utilProto
{
	/**
	 * @brief Parse proto file
	 * @oaram path
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
	)
	{
		if (error_output)
			*error_output << MSG_PARSE_FILE << filename << std::endl;
		return parseProtoFile2(filename, path, messages, error_output);
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

	/**
	 * @brief Each protobuf3 file must have .proto file name suffix
	 * @param path
	 * @param messages
	 * @param error_output std::ostream
	 * @return successfully parsed files count
	 */
	size_t parseProtoFiles
	(
		const std::string &path,
		std::map<std::string, const google::protobuf::Descriptor*> &messages,
		std::ostream *error_output
	)
	{
		std::vector<std::string> protoFiles;
		filesInPath(path, ".proto", &protoFiles);

		size_t r = 0;
		for (std::string fn : protoFiles)
		{
			if (error_output)
				*error_output << fn << ".. ";
			bool ok = utilProto::parseProtoFile(fn.c_str(), path, messages, error_output);
			if (error_output)
				*error_output << (ok ? MSG_OK : MSG_FAILED) << std::endl;
			if (ok)
				r++;
		}
		if (error_output)
				*error_output << r << "/" << protoFiles.size() << " " << MSG_PROCESSED << std::endl;
		return r;
	}

}
