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

#include "message2gateway-config.h"
#include "input-packet.h"
#include "output-message.h"

bool processMessage
(
		google::protobuf::MessageLite* message
)
{
	LOG(INFO) << "Process message " << message->ByteSize();
}

/**
  * Return:  0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
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
		google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
		google::protobuf::MessageLite* message;
		if (!message->MergeFromCodedStream(&input))
		{
			// fatal error occured
			LOG(ERROR) << "Error decoding message, exit.";
			break;
		}
		processMessage(message);
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
