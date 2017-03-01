#include <iostream>
#include <fstream>
#include <string>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

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
#include "errorcodes.h"
#include "utilprotobuf.h"

using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;


uint64_t read_count = 0;
uint64_t sent_count = 0;

void sendMessage
(
		int nnsocket,
		const MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message &message
)
{
	// io::FileOutputStream out(STDOUT_FILENO);
	// TextFormat::Print(*message, &out);
	// std::cout << message->DebugString() << std::endl;

	std::string r = stringDelimitedMessage(messageTypeNAddress, message);
	int sent = nn_send(nnsocket, r.c_str(), r.size(), 0);
	if (sent < 0)
		LOG(ERROR) << ERR_NN_SEND << sent;
}

/**
  * @brief Receives message from stream
  * @return:  0- success
  */
int run_stream
(
		Config *config
)
{
	std::istream *strm;
	if (config->file_name.empty())
	{
		strm = &std::cin;
	}
	else
	{
		strm = new std::ifstream(config->file_name, std::ifstream::in);
	}

	int nano_socket_out = nn_socket(AF_SP, NN_PUB);

	if (nn_connect(nano_socket_out, config->message_out_url.c_str()) < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_out_url;
		return ERRCODE_NN_CONNECT;
	}


	ProtobufDeclarations pd;
	pd.addPath(config->proto_path);

	size_t c = pd.parseProtoPath(config->proto_path);
	std::map<std::string, const google::protobuf::Descriptor*> *messageDescriptors = pd.getMessages();
	if ((messageDescriptors == NULL) || (messageDescriptors->size() == 0))
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

	google::protobuf::io::IstreamInputStream isistream(strm);
    google::protobuf::io::CodedInputStream input(&isistream);
    input.SetTotalBytesLimit(MAX_PROTO_TOTAL_BYTES_LIMIT, -1);

    while (!config->stop_request)
    {
		if (strm->eof())
		{
			LOG(INFO) << MSG_EOF << " " << read_count;
			break;
		}
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, &input, &messageTypeNAddress);
		if (m)
		{
			sendMessage(nano_socket_out, &messageTypeNAddress, *m);
		}
		else
		{
			LOG(ERROR) << ERR_DECODE_MESSAGE;
		}
		read_count++;
    }

    LOG(INFO) << MSG_LOOP_EXIT;

	if (strm && (!config->file_name.empty()))
	{
		delete strm;
	}

	int r = nn_shutdown(nano_socket_out, 0);
	return r;
}

/**
  * @brief Receives message from nanomsg socket
  * @return:  0- success
  */
int run_socket
(
		Config *config
)
{
	int nano_socket_in = nn_socket(AF_SP, NN_SUB);
	if (nn_connect(nano_socket_in, config->message_in_url.c_str()) < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_in_url;
		return ERRCODE_NN_CONNECT;
	}

	int nano_socket_out = nn_socket(AF_SP, NN_PUB);
	if (nn_connect(nano_socket_out, config->message_out_url.c_str()) < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_out_url;
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd;
	pd.addPath(config->proto_path);
	
	std::map<std::string, const google::protobuf::Descriptor*> *messageDescriptors = NULL;
	
	if (pd.parseProtoPath(config->proto_path))
	{
		messageDescriptors = pd.getMessages();	
	}
	
	if ((messageDescriptors == NULL) || (messageDescriptors->size() == 0))
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

	// pd.debug(messageDescriptors);

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY << config->buffer_size;
		return ERRCODE_NO_MEMORY;
	}

    while (!config->stop_request)
    {
    	int bytes = nn_recv(nano_socket_in, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
    		LOG(ERROR) << ERR_NN_RECV << bytes;
    		continue;
    	}
    	else
    		if (bytes == 0)
    			continue;

    	google::protobuf::io::ArrayInputStream isistream(buffer, bytes);
        google::protobuf::io::CodedInputStream input(&isistream);
        input.SetTotalBytesLimit(MAX_PROTO_TOTAL_BYTES_LIMIT, -1);

		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, &input, &messageTypeNAddress);
		if (m)
		{
			sendMessage(nano_socket_out, &messageTypeNAddress, *m);
		}
		else
		{
			LOG(ERROR) << ERR_DECODE_MESSAGE;
		}
    }

    return nn_shutdown(nano_socket_out, 0) | nn_shutdown(nano_socket_in, 0);
}


/**
  * @brief Receives message from stream or nanomsg socket
  * @return:  0- success
  */
int run
(
		Config *config
)
{
	if (config->message_in_url.empty())
		return run_stream(config);
	else
		return run_socket(config);
}

/**
  * @return 0- success
  *        1- config is not initialized yet
  */
int stop
(
		Config *config
)
{
    if (!config)
        return ERRCODE_STOP;
    config->stop_request = true;
    return ERR_OK;
    // wake up

}
