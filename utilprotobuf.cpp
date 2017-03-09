#include <string>
#include <string.h>
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

MessageTypeNAddress::MessageTypeNAddress()
	: message_type(""), message_size(0)
{
    memset(&socket_address_src, sizeof(struct sockaddr), 0);
    memset(&socket_address_dst, sizeof(struct sockaddr), 0);
}

MessageTypeNAddress::MessageTypeNAddress
(
		const std::string &messagetype
)
	: message_type(messagetype), message_size(0)
{
    memset(&socket_address_src, sizeof(struct sockaddr), 0);
    memset(&socket_address_dst, sizeof(struct sockaddr), 0);
}

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
	google::protobuf::compiler::DiskSourceTree source_tree;

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
		for (std::map<std::string, const google::protobuf::Descriptor*>::const_iterator it(messages->begin()); it != messages->end(); ++it)
		{
			std::cout << it->first << " => " << it->second->DebugString() << std::endl;
		}
	}

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
	)
	{
		size_t r = 0;
		for (int i = 0; i < protoFiles.size(); i++)
		{
			if (error_output)
				*error_output << protoFiles[i] << ".. ";
			bool ok = utilProto::parseProtoFile(protoFiles[i].c_str(), path, messages, error_output);
			if (error_output)
				*error_output << (ok ? MSG_OK : MSG_FAILED) << std::endl;
			if (ok)
				r++;
		}
		if (error_output)
				*error_output << r << "/" << protoFiles.size() << " " << MSG_PROCESSED << std::endl;
		return r;
	}

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
	)
	{
		std::vector<std::string> protoFiles;
		filesInPath(path, ".proto", 2, &protoFiles);
		return parseProtoFiles(path, protoFiles, messages, error_output);
	}

}

/**
 * @brief Write Message type string, size of message and message itself
 *
 * @param messageTypeName
 * @param message
 * @param rawOutput
 * @return
 */
int writeDelimitedMessage
(
	const MessageTypeNAddress *messageTypeNAddress,
    const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput
)
{
	// We create a new coded stream for each message.  Don't worry, this is fast.
	google::protobuf::io::CodedOutputStream *output = new google::protobuf::io::CodedOutputStream(rawOutput);

	output->WriteRaw(&messageTypeNAddress->socket_address_src, SOCKADDR_SIZE);
	output->WriteRaw(&messageTypeNAddress->socket_address_dst, SOCKADDR_SIZE);
	// Write the type
	output->WriteVarint32(messageTypeNAddress->message_type.size());
	output->WriteString(messageTypeNAddress->message_type);
	// Write the size.
	const int size = message.ByteSize();
	output->WriteVarint32(size);
	uint8_t* buffer = output->GetDirectBufferForNBytesAndAdvance(size);

	int r = (4 + messageTypeNAddress->message_type.size() + 4 + size);
	if (buffer != NULL)
	{
		// Optimization:  The message fits in one buffer, so use the faster direct-to-array serialization path.
		message.SerializeWithCachedSizesToArray(buffer);
	}
	else
	{
		// Slightly-slower path when the message is multiple buffers.
		message.SerializeWithCachedSizes(output);
		if (output->HadError())
		{
			r = -1;
		}
	}

	delete output;
	return r;
}

/**
 * @brief Write Message type string, size of message and message itself to the string
 * @param messageTypeName
 * @param message
 * @return
 */
std::string stringDelimitedMessage
(
	const MessageTypeNAddress *messageTypeNAddress,
    const google::protobuf::MessageLite& message
)
{
	std::stringstream ss(std::stringstream::out | std::stringstream::binary);
	google::protobuf::io::OstreamOutputStream *strm = new google::protobuf::io::OstreamOutputStream(&ss);
	int sz = writeDelimitedMessage(messageTypeNAddress, message, strm);
	delete strm;
	return ss.str();
}

/**
 * Read delimited message from the coded stream
 * @param strm
 * @param messageType
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		google::protobuf::io::CodedInputStream *input,
		MessageTypeNAddress *messageTypeNAddress
)
{
	input->ReadRaw(&messageTypeNAddress->socket_address_src, SOCKADDR_SIZE);
	input->ReadRaw(&messageTypeNAddress->socket_address_dst, SOCKADDR_SIZE);
	uint32_t message_type_size;
	input->ReadVarint32(&message_type_size);
	input->ReadString(&messageTypeNAddress->message_type, message_type_size);
	input->ReadVarint32(&messageTypeNAddress->message_size);
	google::protobuf::io::CodedInputStream::Limit limit = input->PushLimit(messageTypeNAddress->message_size);
	// google::protobuf::Arena arena;
	// Message *m = pd->decode(&arena, messageTypeNAddress->message_type, input);
	Message *m = pd->decode(messageTypeNAddress->message_type, input);
	if (!m)
		input->Skip(messageTypeNAddress->message_size);
	input->ConsumedEntireMessage();
	input->PopLimit(limit);
	return m;
}

/**
 * Read delimited message from the stream
 * @param pd
 * @param strm
 * @param messageType
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
	ProtobufDeclarations *pd,
	std::istream *strm,
	MessageTypeNAddress *messageTypeNAddress
)
{
	google::protobuf::io::IstreamInputStream isistream(strm);
    google::protobuf::io::CodedInputStream input(&isistream);
    return readDelimitedMessage(pd, &input, messageTypeNAddress);
}

/**
 * Read delimited message from the string
 * @param value
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		std::string &buffer,
		MessageTypeNAddress *messageTypeNAddress
)
{
	std::stringstream ss(buffer);
	return readDelimitedMessage(pd, &ss, messageTypeNAddress);
}

/**
 * Read delimited message from the buffer
 * @param buffer
 * @param size
 * @return message
 */
google::protobuf::Message *readDelimitedMessage
(
		ProtobufDeclarations *pd,
		void *buffer,
		int size,
		MessageTypeNAddress *messageTypeNAddress
)
{
	google::protobuf::io::ArrayInputStream isistream(buffer, size);
	google::protobuf::io::CodedInputStream input(&isistream);
	input.SetTotalBytesLimit(MAX_PROTO_TOTAL_BYTES_LIMIT, -1);
	return readDelimitedMessage(pd, &input, messageTypeNAddress);
}

