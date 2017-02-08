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

const std::string fileNameSuffixProto(".proto");

std::map<std::string, const google::protobuf::Descriptor*> internalMessages;

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;


bool parseProtoFile
(
		int file_descriptor,
		std::map<std::string, const google::protobuf::Descriptor*> &messages
)
{
	FileInputStream proto_stream(file_descriptor);
	Tokenizer input_proto(&proto_stream, NULL);

	FileDescriptorProto file_desc_proto;
	Parser parser;
	return parser.Parse(&input_proto, &file_desc_proto);

	google::protobuf::DescriptorPool pool;
	const google::protobuf::FileDescriptor* file_desc = pool.BuildFile(file_desc_proto);
	if (file_desc == NULL)
	{
		std::cerr << "Cannot get file descriptor from file descriptor proto " << file_desc_proto.DebugString();
		return false;
	}

	int count = file_desc->message_type_count();
	for (int m = 0; m < count; m++)
	{
		const google::protobuf::Descriptor* md = file_desc->message_type(m);
		std::cerr << "message: " << md->DebugString() << std::endl;
		messages[md->name()] = md;
	}
	return true;
}

bool parseProtoFile
(
		FILE *file,
		std::map<std::string, const google::protobuf::Descriptor*> &messages
)
{
	return parseProtoFile(fileno(file), messages);
}

namespace utilProto
{

	/// Each protobuf3 file must have .proto file name suffix
	bool parseProtoFile
	(
		const char *path,
		std::map<std::string, const google::protobuf::Descriptor*> &messages
	)
	{
		bool r = false;
		FILE *f = fopen(path, "r");
		if (f != NULL)
		{
			r = parseProtoFile(f, messages);
			fclose(f);
		}
		return r;
	}

	/**
	 * for nftw() use only
	 */
	bool parseProtoFile
	(
		const char *path
	)
	{
		return parseProtoFile(path, internalMessages);
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
