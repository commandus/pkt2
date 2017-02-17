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

	// io::FileOutputStream out(STDOUT_FILENO);
	// TextFormat::Print(*message, &out);
	std::cout << message->DebugString() << std::endl;
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

	// pd.debug(messageDescriptors);


	google::protobuf::io::IstreamInputStream isistream(strm);
    google::protobuf::io::CodedInputStream input(&isistream);

    input.SetTotalBytesLimit(512 * 1024 * 1024, -1);

    while (!config->stop_request)
    {
    	if (strm->eof())
    	{
    		LOG(INFO) << "End of file detected, exit.";
    		break;
    	}

    	uint32_t message_type_size;
    	input.ReadVarint32(&message_type_size);
    	std::string messageType;
    	input.ReadString(&messageType, message_type_size);
		uint32_t size;
		input.ReadVarint32(&size);
		google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
		Message *m = pd.decode(messageType, &input);
		if (m)
		{
			processMessage(m);
		}
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
