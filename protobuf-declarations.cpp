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

#include "pbjson.hpp"
#include "pkt2.pb.h"
#include "utilfile.h"
#include "utilstring.h"

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
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/parser.h>

#include <glog/logging.h>

#include "errorcodes.h"

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

const std::string fileNameSuffixProto = (".proto");

/**
 * @brief After addPath() and parseProtoPath()
 */
ProtobufDeclarations::ProtobufDeclarations()
	: verbosity(0)
{
	// Allocate the Importer.
	importer = new Importer(&source_tree, &mf_error_printer);
	dynamic_factory = new DynamicMessageFactory(importer->pool());
}

/**
 * @brief Add virtual path "" and parse all files recursively
 * @param path
 */
ProtobufDeclarations::ProtobufDeclarations(
	const std::string &path,
	int verbose
)
	: verbosity(verbose)
{
	// Allocate the Importer.
	importer = new Importer(&source_tree, &mf_error_printer);
	dynamic_factory = new DynamicMessageFactory(importer->pool());
	addPath("", path);
	parseProtoPath(path);
}

/**
 * @brief add .proto include path
 * @param virtual_path virtual path
 * @param path include path
 */
void ProtobufDeclarations::addPath
(
		const std::string &virtual_path,
		const std::string &path
)
{
	paths.push_back(path);
	source_tree.MapPath(virtual_path, path);
}

/**
 * @brief add .proto include path
 * @param path include path
 */
void ProtobufDeclarations::addPath
(
		const std::string &path
)
{
	addPath("", path);
}

ProtobufDeclarations::~ProtobufDeclarations() {
	if (dynamic_factory)
		delete dynamic_factory;
	if (importer)
		delete importer;
}

std::map<std::string, const google::protobuf::Descriptor*> *ProtobufDeclarations::getMessages()
{
	return &internalMessages;
}

size_t ProtobufDeclarations::getMessageCount()
{
	return internalMessages.size();
}

/**
 * Return message descriptor by the name
 * @param name
 * @return NULL if message name not found
 */
const google::protobuf::Descriptor* ProtobufDeclarations::getMessageDescriptor
(
	const std::string &name
) const
{
	std::map<std::string, const google::protobuf::Descriptor*>::const_iterator it(internalMessages.find(name));
	if (it == internalMessages.end())
		return NULL;
	else
		return it->second;
}

/**
 * Get a new message
 * @param message_name
 * @return NULL if not found
 */
Message *ProtobufDeclarations::getMessage
(
	const std::string &message_name
) const
{
	// Look up the type.
	const Descriptor* type = importer->pool()->FindMessageTypeByName(message_name);
	if (type == NULL)
		return NULL;

	Message *message(dynamic_factory->GetPrototype(type)->New());
	return message;
}

/**
 * @brief decode message from the stream
 * @param message_name Protobuf message name
 * @param stream
 * @return decoded Protobuf message from the stream
 */
Message *ProtobufDeclarations::decode
(
	const std::string &message_name,
	google::protobuf::io::CodedInputStream *stream
)
{
	Message *message = getMessage(message_name);
	if (!message)
		return NULL;

	// Input is binary.
	if (!message->ParsePartialFromCodedStream(stream))
	{
		// delete message;
		return NULL;
	}
	return message;
}

/**
 * @brief decode message from the stream
 * @param arena protobuf arena
 * @param message_name Protobuf message name
 * @param stream
 * @return decoded Protobuf message from the stream
 */
Message *ProtobufDeclarations::decode
(
	google::protobuf::Arena *arena,
	const std::string &message_name,
	google::protobuf::io::CodedInputStream *stream
)
{
	// Look up the type.
	const Descriptor* type = importer->pool()->FindMessageTypeByName(message_name);
	if (type == NULL)
		return NULL;

	Message *message(dynamic_factory->GetPrototype(type)->New(arena));

	// Input is binary.
	if (!message->ParsePartialFromCodedStream(stream))
	{
		// delete message;
		return NULL;
	}
	return message;
}

/**
 * @brief decode message from the stream
 * @param message_name Protobuf message name
 * @param stream
 * @return decoded Protobuf message from the stream
 */
Message *ProtobufDeclarations::decode
(
	const std::string &message_name,
	google::protobuf::io::IstreamInputStream *stream
)
{
	// Look up the type
	const Descriptor* type = importer->pool()->FindMessageTypeByName(message_name);
	if (type == NULL)
		return NULL;

	Message *message(dynamic_factory->GetPrototype(type)->New());

	// Input is binary.
	if (!message->ParsePartialFromZeroCopyStream(stream))
	{
		delete message;
		return NULL;
	}
	return message;
}

#ifdef LIB_PBJSON
/**
 * Decode JSON data
 * @param message_name message name
 * @param json message value
 * @return NULL if no protobuf found or error occurred
 */
google::protobuf::Message *ProtobufDeclarations::decode
(
	const std::string &message_name,
	const std::string &json
)
{
	// Look up the type
	const Descriptor* type = importer->pool()->FindMessageTypeByName(message_name);
	if (type == NULL)
		return NULL;

	Message *message(dynamic_factory->GetPrototype(type)->New());

	if (message)
	{
		std::string err;
		if (pbjson::json2pb(json, message, err))
		{
			LOG(ERROR) << ERR_INVALID_JSON << " " << err << ": " << json;
			return NULL;
		}
	}
	return message;
}
#else
#error pbjson library missed 
#endif

/**
 * Encode message to the packet
 * @param buffer can be NULL. Return size
 * @param message
 * @return NULL if no protobuf found or error occurred
 */
size_t ProtobufDeclarations::encode
(
	void **buffer,
	const google::protobuf::Message * message
)
{
	return 0;
}

bool ProtobufDeclarations::parseProtoFile
(
	const char *fn
)
{
	// Import the file
	importer->AddUnusedImportTrackFile(fn);
	const FileDescriptor* parsed_file = importer->Import(fn);
	importer->ClearUnusedImportTrackFiles();
	if (parsed_file == NULL)
	{
		LOG(ERROR) << ERR_LOAD_PROTO << fn;
		return false;
	}
	parsed_files.push_back(parsed_file);

	FILE *f = openProto(fn);
	if (f == NULL)
	{
		LOG(ERROR) << ERR_OPEN_PROTO << fn;
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
		LOG(ERROR) << ERR_PARSE_PROTO << fn;
		return false;
	}

	// get "top level" file and print out messages
	const FileDescriptor *file_desc = importer->pool()->FindFileByName(fn);

	if (file_desc == NULL)
	{
		LOG(ERROR) << ERR_PROTO_GET_DESCRIPTOR << fn;
		fclose(f);
		return false;
	}

	int count = file_desc->message_type_count();
	for (int m = 0; m < count; m++)
	{
		const google::protobuf::Descriptor* md = file_desc->message_type(m);
		internalMessages[file_desc->package() + "." + md->name()] = md;
	}

	fclose(f);

	return true;
}

/**
 * parse proto files
 * @param protoFiles
 * @return count successfully parsed files
 */
size_t ProtobufDeclarations::parseProtoFiles
(
	const std::vector<std::string> &protoFiles
)
{
	size_t r = 0;
	for (int i = 0; i < protoFiles.size(); i++)
	{
		if (parseProtoFile(protoFiles[i].c_str()))
			r++;
	}
	return r;

}

/**
 * @brief Each protobuf3 file must have .proto file name suffix
 * @param path
 * @return successfully parsed files count
 */
size_t ProtobufDeclarations::parseProtoPath
(
	const std::string &path
)
{
	std::vector<std::string> protoFiles;
	filesInPath(path, ".proto", 2, &protoFiles);

	if (verbosity >= 2)
	{
		LOG(INFO) << MSG_PROTO_FILES_HEADER;
		for (std::vector<std::string>::iterator iter = protoFiles.begin(); iter != protoFiles.end(); ++iter)
			LOG(INFO) << *iter;
	}

	size_t r = parseProtoFiles(protoFiles);

	if (verbosity >= 2)
	{
		LOG(INFO) << MSG_PARSE_PROTO_COUNT << r;
		LOG(INFO) << MSG_MESSAGE_HEADER;
		for (std::map<std::string, const google::protobuf::Descriptor*>::iterator iter = internalMessages.begin(); iter != internalMessages.end(); ++iter)
			LOG(INFO) << iter->first;
	}
	return r;
}

/**
 * @brief Try add path from include paths at specified index to the file name and open
 * @param fn file name to concatenate
 * @param index path index
 * @return
 */
FILE *ProtobufDeclarations::openProto
(
		const std::string &fn
)
{
	FILE *f = fopen(fn.c_str(), "r");
	if (f == NULL)
	{
		for (int i = 0; i < paths.size(); i++)
		{
			std::string fn1 = paths[i] + "/" + fn;
			f = fopen(fn1.c_str(), "r");
			if (f != NULL)
				break;
		}
	}
	return f;
}

/**
 * Print out messages to stdout
 * @param messages
 */
void ProtobufDeclarations::debugPrint
(
	const std::map<std::string, const google::protobuf::Descriptor*> *messages
)
{
	for (std::map<std::string, const google::protobuf::Descriptor*>::const_iterator it(messages->begin()); it != messages->end(); ++it)
	{
		std::cout << it->first << " => " << it->second->DebugString() << std::endl;
	}
}

/**
 * @brief get SQL CREATE TABLE statements for message
 * @param messages
 */
int ProtobufDeclarations::getStatementSQLCreate
(
	std::vector<std::string> *retval,
	int mode,
	const google::protobuf::Descriptor* message_descriptor
)
{
	if (!message_descriptor)
        return ERRCODE_DECOMPOSE_NO_MESSAGE_DESCRIPTOR;

	size_t count = message_descriptor->field_count();
	std::stringstream ss;
	ss << "CREATE TABLE \"" << replace(message_descriptor->full_name(), ".", "_") << "\"(";
	for (size_t i = 0; i != count; ++i)
	{
		const google::protobuf::FieldDescriptor *field = message_descriptor->field(i);
		if (!field)
			return ERRCODE_DECOMPOSE_NO_FIELD_DESCRIPTOR;

		ss << field->name() << " ";

		switch (field->cpp_type()) {
			case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
				ss << "VARCHAR(32)";
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				ss << "FLOAT";
				break;
			default:
				ss << "INTEGER";
				break;
		}

		/*
		const google::protobuf::FieldOptions foptions = field->options();
		pkt2::Variable variable = foptions.GetExtension(pkt2::variable);
		*/

		ss << ", ";
	}
	ss << "id bigint);";

	retval->push_back(ss.str());
	return ERR_OK;
}

/**
 * @brief get SQL CREATE TABLE statements for message
 * @param messages
 */
int ProtobufDeclarations::getStatementSQLCreate
(
	std::vector<std::string> *retval,
	int mode,
	const std::string &message_name
)
{
	return getStatementSQLCreate(retval, mode, getMessageDescriptor(message_name));
}

/**
 * @brief get SQL CREATE TABLE statements for all messages
 * @param messages
 */
int ProtobufDeclarations::getStatementSQLCreate
(
	std::vector<std::string> *retval,
	int mode
)
{
	for (std::map<std::string, const google::protobuf::Descriptor*>::const_iterator it(internalMessages.begin()); it != internalMessages.end(); ++it)
	{
		getStatementSQLCreate(retval, mode, it->second);
	}
	return ERR_OK;
}
