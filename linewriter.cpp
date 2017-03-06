/**
 *
 */
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "linewriter.h"
#include "output-message.h"

#include "errorcodes.h"
#include "utilprotobuf.h"

#include "json/json.h"
#include "pbjson.hpp"

#include "pkt2packetvariable.cpp"

using namespace google::protobuf;

/**
 * @brief Print packet to the stdout
 * @param buffer
 * @param buffer_size
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_debug
(
		void *buffer,
		int buffer_size,
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	std::cout << message->DebugString() << std::endl;
	return 0;
}

/**
 * @brief Print packet to the stdout
 * @param buffer
 * @param buffer_size
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_json
(
		void *buffer,
		int buffer_size,
		MessageTypeNAddress *messageTypeNAddress,
		const google::protobuf::Message *message
)
{
	std::string out;
	pbjson::pb2json(message, out);
	std::cout << out << std::endl;
	return ERR_OK;
}

/**
 * @brief Print packet's message options to the stdout
 * @param buffer
 * @param buffer_size
 * @param messageTypeNAddress
 * @param message
 * @return 0 - success
 */
int put_pkt2_options
(
		void *buffer,
		int buffer_size,
		ProtobufDeclarations *pd,
		MessageTypeNAddress *messageTypeNAddress
)
{
	// Each message
	const google::protobuf::Descriptor* md = pd->getMessageDescriptor(messageTypeNAddress->message_type);
	if (!md)
	{
		LOG(ERROR) << ERR_MESSAGE_TYPE_NOT_FOUND << messageTypeNAddress->message_type;
		return ERRCODE_MESSAGE_TYPE_NOT_FOUND;
	}

	const google::protobuf::MessageOptions options = md->options();
	if (options.HasExtension(pkt2::packet))
	{
		std::string out;
		pkt2::Packet packet =  options.GetExtension(pkt2::packet);
		pbjson::pb2json(&packet, out);
		std::cout << "{\"packet\":" << out << "," << std::endl;
	}

	std::cout << "\"variables\":[";
	for (int f = 0; f < md->field_count(); f++)
	{
		const google::protobuf::FieldOptions foptions = md->field(f)->options();
		std::string out;
		pkt2::Variable variable = foptions.GetExtension(pkt2::variable);
		pbjson::pb2json(&variable, out);
		std::cout << out;
		if (f < md->field_count() - 1)
			std::cout << ",";
	}
	std::cout << "]}" << std::endl;

	return ERR_OK;
}


/**
 * @brief Write line loop
 * @param config
 * @return  0- success
 *          >0- error (see errorcodes.h)
 */
int run
(
		Config *config
)
{
	int nano_socket = nn_socket(AF_SP, NN_SUB);
	int r = nn_setsockopt(nano_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SUBSCRIBE << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SUBSCRIBE;
	}
	int eid = nn_connect(nano_socket, config->message_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd(config->proto_path);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY;
		return ERRCODE_NO_MEMORY;
	}

	while (!config->stop_request)
    {

    	int bytes = nn_recv(nano_socket, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
    		LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, buffer, bytes, &messageTypeNAddress);
		if (m)
		{
			switch (config->mode)
			{
			case 0:
				put_json(buffer, bytes, &messageTypeNAddress, m);
				break;
			case 2:
				put_pkt2_options(buffer, bytes, &pd, &messageTypeNAddress);
				break;
			default:
				put_debug(buffer, bytes, &messageTypeNAddress, m);
			}
    	}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE;
    }

    free(buffer);

	r = nn_shutdown(nano_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;

	}
	return r;
}

/**
 *  @brief Stop writer
 *  @param config
 *  @return 0- success
 *          >0- config is not initialized yet
 */
int stop
(
		Config *config
)
{
    if (!config)
    {
    	LOG(ERROR) << ERR_STOP;
        return ERRCODE_STOP;
    }
    config->stop_request = true;
    // wake up
    return ERR_OK;
}
