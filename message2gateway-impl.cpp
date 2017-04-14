#include <iostream>
#include <fstream>
#include <string>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

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
#include "packet2message.h"
#include "errorcodes.h"
#include "utilstring.h"
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
	if (config->from_stdin || config->file_name.empty())
		strm = &std::cin;
	else
		strm = new std::ifstream(config->file_name, std::ifstream::in);

	int nano_socket_out = nn_socket(AF_SP, NN_BUS);

	int eoutid = nn_bind(nano_socket_out, config->message_out_url.c_str());
	if (eoutid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_out_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd(config->proto_path, config->verbosity);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

	IstreamInputStream isistream(strm);
    CodedInputStream input(&isistream);
    input.SetTotalBytesLimit(MAX_PROTO_TOTAL_BYTES_LIMIT, -1);

    while (!config->stop_request)
    {
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, &input, &messageTypeNAddress);
		if (m)
		{
			for (int i = 0; i < config->retries; i++)
			{
				sendMessage(nano_socket_out, &messageTypeNAddress, *m);
				if (i < config->retries - 1)
					sleep(config->retry_delay);
			}
		}
		else
		{
			LOG(INFO) << read_count << " sent";
			break;
		}
		read_count++;
		sleep(config->retry_delay);
    }

    LOG(INFO) << MSG_LOOP_EXIT << read_count << " sent";

	if (strm && (!config->file_name.empty()))
	{
		delete strm;
	}

	int r = nn_shutdown(nano_socket_out, eoutid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_out_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;

	}
	return r;
}

/**
  * @brief Receives message not a packet!)from nanomsg socket from socket ipc:///tmp/packet.pkt2, write to socket ipc:///tmp/message.pkt2
  * @return:  0- success
  */
int run_socket
(
		Config *config
)
{
	int nano_socket_in = nn_socket(AF_SP, NN_BUS);
	int einid = nn_connect(nano_socket_in, config->message_in_url.c_str());
	if (einid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_in_url << " " << errno << " " << strerror(errno);;
		return ERRCODE_NN_CONNECT;
	}

	int nano_socket_out = nn_socket(AF_SP, NN_BUS);
	int eoutid = nn_bind(nano_socket_out, config->message_out_url.c_str());
	if (eoutid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_out_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd(config->proto_path, config->verbosity);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY << config->buffer_size;
		return ERRCODE_NO_MEMORY;
	}

    ProtobufDeclarations declarations(config->proto_path, config->verbosity);
    Pkt2OptionsCache options_cache(&declarations);
    Packet2Message packet2Message(&declarations, &options_cache, config->verbosity);

    while (!config->stop_request)
    {
    	int bytes = nn_recv(nano_socket_in, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
    		LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
    	else
    		if (bytes == 0)
    			continue;

        // packet -> message
    	MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, buffer, bytes, &messageTypeNAddress);
		if (m)
			sendMessage(nano_socket_out, &messageTypeNAddress, *m);
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE;

    	sleep(0);	// BUGBUG Pass 0 for https://github.com/nanomsg/nanomsg/issues/182
    }

	free(buffer);

    int r = nn_shutdown(nano_socket_out, eoutid) | nn_shutdown(nano_socket_in, einid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_in_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}
	return r;
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
	if (config->from_stdin || (!config->file_name.empty()) || config->message_in_url.empty())
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
