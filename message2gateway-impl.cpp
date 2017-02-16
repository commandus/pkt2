#include <iostream>
#include <fstream>
#include <string>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>

#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>

#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/compiler/parser.h>

#include "message2gateway-config.h"
#include "input-packet.h"
#include "output-message.h"
#include "protobuf-declarations.h"

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool processMessage
(
		google::protobuf::Message* message
)
{
	LOG(INFO) << "Process message " << message->DebugString();
}

/**
  * Return:  0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- protobuf load messages error occurred
  *          4- send error, re-open
  *          5- LMDB open database file error
  */
int run
(
		Config *config
)
{
	int nano_socket = nn_socket(AF_SP, NN_PUSH);
	if (nn_connect(nano_socket, config->message_url.c_str()) < 0)
	{
		LOG(ERROR) << "Can not connect to the IPC url " << config->message_url;
		return 2;
	}

	std::istream *strm;
	if (config->file_name.empty())
	{
		strm = &std::cin;
	}
	else
	{
		strm = new std::ifstream(config->file_name, std::ifstream::in);
	}

	ProtobufDeclarations pd("proto");
	
	std::map<std::string, const google::protobuf::Descriptor*> *messageDescriptors = NULL;
	
	if (pd.parseProtoFile("example/example1.proto"))
	{
		messageDescriptors = pd.getMessages();	
	}
	
	if ((messageDescriptors == NULL) || (messageDescriptors->size() == 0))
	{
		LOG(ERROR) << "Can not load proto files from ";
		return 3;
	}

	pd.debug(messageDescriptors);


	google::protobuf::io::IstreamInputStream isistream(strm);
    google::protobuf::io::CodedInputStream input(&isistream);

    while (!config->stop_request)
    {
    	if (strm->eof())
    	{
    		LOG(INFO) << "End of file detected, exit.";
    		break;
    	}
		uint32_t size;
		input.ReadVarint32(&size);
		LOG(ERROR) << size;
		google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);














/*
		FileInputStream proto_stream(0);
		Tokenizer input_proto(&proto_stream, NULL);

		FileDescriptorProto file_desc_proto;
		Parser parser;
		if (!parser.Parse(&input_proto, &file_desc_proto)) {
			LOG(ERROR) << "Failed to parse .proto definition";
			break;
		}

		// Set the name in file_desc_proto as Parser::Parse does not do this:
		std::string message_type("TemperaturePkt");
		if (!file_desc_proto.has_name()) {
			file_desc_proto.set_name(message_type);
		}

		google::protobuf::DescriptorPool pool;
		const google::protobuf::FileDescriptor* file_desc = pool.BuildFile(file_desc_proto);
		if (file_desc == NULL) {
			std::cerr << "Cannot get file descriptor from file descriptor proto"
					<< file_desc_proto.DebugString();
			return -1;
		}

		const google::protobuf::Descriptor* message_desc = file_desc->FindMessageTypeByName(message_type);
		if (message_desc == NULL) {
			LOG(ERROR) << "Cannot get message descriptor of message: " << message_type;
			break;
		}

		google::protobuf::DynamicMessageFactory factory;
		const google::protobuf::Message* prototype_msg = factory.GetPrototype(message_desc); // prototype_msg is immutable
		if (prototype_msg == NULL) {
			LOG(ERROR) << "Cannot create prototype message from message descriptor";
			break;
		}

		google::protobuf::Message* mutable_msg = prototype_msg->New();
		if (mutable_msg == NULL) {
			LOG(ERROR) << "Failed in prototype_msg->New(); to create mutable message";
			break;
		}
*/

















/*
		if (!mutable_msg->MergeFromCodedStream(&input))
		{
			// fatal error occured
			LOG(ERROR) << "Error decoding message, exit.";
			break;
		}
		processMessage(mutable_msg);
		*/
		input.ConsumedEntireMessage();
		input.PopLimit(limit);
    }

	if (strm && (!config->file_name.empty()))
	{
		delete strm;
	}

	return nn_shutdown(nano_socket, 0);
}

/**
  * Return 0- success
  *        1- config is not initialized yet
  */
int stop
(
		Config *config
)
{
    if (!config)
        return 1;
    config->stop_request = true;
    // wake up

}
