#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <algorithm>

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

#ifdef ENABLE_LOG
#include <glog/logging.h>
#endif

#include "platform.h"
#include "error-printer.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "utilfile.h"

MessageTypeNAddress::MessageTypeNAddress()
	: message_type(""), message_size(0)
{
    memset(&socket_address_src, 0, sizeof(struct sockaddr));
    memset(&socket_address_dst, 0, sizeof(struct sockaddr));
}

MessageTypeNAddress::MessageTypeNAddress
(
		const std::string &messagetype
)
	: message_type(messagetype), message_size(0)
{
    memset(&socket_address_src, 0, sizeof(struct sockaddr));
    memset(&socket_address_dst, 0, sizeof(struct sockaddr));
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

	std::vector<const FileDescriptor*> parsed_files;

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
		pkt2utilfile::filesInPath(path, ".proto", 2, &protoFiles);
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
	const int size = message.ByteSizeLong();	// ByteSize() deprecated
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
	input.SetTotalBytesLimit(MAX_PROTO_TOTAL_BYTES_LIMIT); //  SetTotalBytesLimit(MAX_PROTO_TOTAL_BYTES_LIMIT, -1); deprecated
	return readDelimitedMessage(pd, &input, messageTypeNAddress);
}

/**
 * @brief Get field value from the packet
 * @param packet data packet containing field
 * @param field field 
 * @return data as string
 */
std::string extractField
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	int sz = packet.size();
	if (sz < field.offset() + field.size())
	{
#ifdef ENABLE_LOG
		LOG(ERROR) << ERR_PACKET_TOO_SMALL << field.name() << " offset: " << field.offset() << ", size : " << field.size() 
			<< ". Packet (size: " << sz << ") lacks " << (field.offset() + field.size() - sz) << " bytes";
#endif				
		std::string r = "";
		r.resize(field.size());
		return r;
	}
	std::string r = packet.substr(field.offset(), field.size());

	if ((field.size() > 1) && ENDIAN_NEED_SWAP(field.endian()))
	{
		char *p = &r[0];
		std::reverse(p, p + field.size());
	}
	return r;
}

/**
 * @brief Get field value from the packet as 64 bit unsigned integer
 * @param packet data packet containing field
 * @param field field 
 * @return integer
 */
uint64_t extractFieldUInt
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 0:
		return 0;
	case 1:
		return *((uint8_t*) &r[0]);
	case 2:
		return *((uint16_t*) &r[0]);
	case 4:
		return *((uint32_t*) &r[0]);
	case 8:
		return *((uint64_t*) &r[0]);
	default:
	{
		uint64_t v = 0;
		memmove(&v, &r[0], (sz < sizeof(uint64_t) ? sz : sizeof(uint64_t)));
		return v;
	}
	}
	return 0;
}

/**
 * @brief Get field value from the packet as 64 bit signed integer
 * @param packet data packet containing field
 * @param field field 
 * @return integer
 */
int64_t extractFieldInt
(
		const std::string &packet,
		const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 0:
		return 0;
	case 1:
		return *((int8_t*) &r[0]);
	case 2:
		return *((int16_t*) &r[0]);
	case 4:
		return *((int32_t*) &r[0]);
	case 8:
		return *((int64_t*) &r[0]);
	default:
	{
		int64_t v = 0;
		memmove(&v, &r[0], (sz < sizeof(int64_t) ? sz : sizeof(int64_t)));
		return v;
	}
	}
	return 0;
}

/**
 * @brief Set field value in the packet from the string
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 */
void setFieldString
(
	std::string &packet,
	const pkt2::Field &field,
	const std::string &value
)
{
	int sz = field.size();
	if (sz == 0)
		return;
	memmove((char *) packet.c_str() + field.offset(), &value, (sz <= field.size() ? sz : field.size()));
}

/**
 * @brief Set field value in the packet from 64 bit integer
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 * @param value integer
 */
void setFieldInt
(
	std::string &packet,
	const pkt2::Field &field,
	int64_t value
)
{
	int sz = field.size();
	if (sz == 0)
		return;
	if ((sz > 1) && ENDIAN_NEED_SWAP(field.endian()))
	{
		char *p = (char *) &value;
		std::reverse(p, p + field.size());
	}
	memmove((char *) packet.c_str() + field.offset(), &value, (sz < sizeof(int64_t) ? sz : sizeof(int64_t)));
}

/**
 * @brief Set field value in the packet from 64 bit integer
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 * @param value unsigned integer
 */
void setFieldUInt
(
	std::string &packet,
	const pkt2::Field &field,
	uint64_t value
)
{
	int sz = field.size();
	if (sz == 0)
		return;
	if ((sz > 1) && ENDIAN_NEED_SWAP(field.endian()))
	{
		char *p = (char *) &value;
		std::reverse(p, p + field.size());
	}
	memmove((char *) packet.c_str() + field.offset(), &value, (sz < sizeof(uint64_t) ? sz : sizeof(uint64_t)));
}

/**
 * @brief Set field value in the packet from 64 bit integer
 * @param packet binary data 
 * @param field descriptor of area in binary data: offset, size, bytes order
 */
void setFieldDouble
(
	std::string &packet,
	const pkt2::Field &field,
	double value
)
{
	std::string r = packet;
	int sz = field.size();
	if (sz == 0)
		return;
	if ((sz > 1) && ENDIAN_NEED_SWAP(field.endian()))
	{
		char *p = (char *) &value;
		std::reverse(p, p + field.size());
	}
	switch (field.size())
	{
		case 0:
			break;
		case 4:
			{
				float vl = value;
				memmove((char *) packet.c_str() + field.offset(), &vl, (sz < sizeof(double) ? sz : sizeof(double)));
			}
			break;
		default:
			memmove((char *) packet.c_str() + field.offset(), &value, (sz < sizeof(double) ? sz : sizeof(double)));
	}			
}

/**
 * @brief Return minimum size of the packet
 * @param packet data packet containing field
 * @return required (mimimum) packet size
 */
size_t getPacketSize
(
	const pkt2::Packet &packet
)
{
	size_t r = 0;
	for (int i = 0; i < packet.fields_size(); i++)
	{
		size_t c = packet.fields(i).offset() + packet.fields(i).size();
		if (c > r)
			r = c;
	}
	return r;
};

/**
 * @brief Get field value from the packet as double
 * @param packet data packet containing field
 * @param field field 
 * @return float
 */
double extractFieldDouble
(
	const std::string &packet,
	const pkt2::Field &field
)
{
	std::string r = extractField(packet, field);
	int sz = field.size();
	switch (sz)
	{
	case 4:
		return *((float*) &r[0]);
	case 8:
		return *((double*) &r[0]);
	}
	return 0;
}
