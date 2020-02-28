/**
 *
 */
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <glog/logging.h>
#include <google/protobuf/message.h>
#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include "linewriter.h"
#include "errorcodes.h"
#include "utilstring.h"

using namespace google::protobuf;

/**
 * @brief Write line loop
 * @param config program configuration
 * @return  0- success
 *          >0- error (see errorcodes.h)
 */
int run
(
	Config *config
)
{
START:
	config->stop_request = 0;
	set_format_number(config->format_number);
	
	if (!config->file_name.empty())
		config->stream = new std::ofstream(config->file_name.c_str(), std::ofstream::app);
	else
		config->stream = &std::cout;
	
	int accept_socket = nn_socket(AF_SP, NN_BUS);
	// int r = nn_setsockopt(accept_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (accept_socket < 0)
	{
		LOG(ERROR) << ERR_NN_SOCKET << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SOCKET;
	}
	int eid = nn_connect(accept_socket, config->message_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	ProtobufDeclarations pd(config->proto_path, config->verbosity);
	if (!pd.getMessageCount())
	{
		LOG(ERROR) << ERR_LOAD_PROTO << config->proto_path;
		return ERRCODE_LOAD_PROTO;
	}
	Pkt2OptionsCache options(&pd);

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY;
		return ERRCODE_NO_MEMORY;
	}

	while (!config->stop_request)
    {

    	int bytes = nn_recv(accept_socket, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
			if (errno == EINTR) 
			{
				LOG(ERROR) << ERR_INTERRUPTED;
				config->stop_request = true;
				break;
			}
			else
				LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
		MessageTypeNAddress messageTypeNAddress;
		Message *m = readDelimitedMessage(&pd, buffer, bytes, &messageTypeNAddress);
		if (config->verbosity >= 2)
		{
			LOG(INFO) << MSG_RECEIVED << bytes << ": " << pkt2utilstring::hexString(buffer, bytes) ;
		}

		if (m)
		{
			if (config->allowed_messages.size())
			{
				if (std::find(config->allowed_messages.begin(), config->allowed_messages.end(), m->GetTypeName()) == config->allowed_messages.end())
				{
					LOG(INFO) << MSG_PACKET_REJECTED << m->GetTypeName();
					continue;
				}
			}

			switch (config->mode)
			{
			case MODE_JSON:
				put_json(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_CSV:
				put_csv(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_TAB:
				put_tab(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_SQL:
				put_sql(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_SQL2:
				put_sql2(config->stream, &options, &messageTypeNAddress, m);
				break;
			case MODE_PB_TEXT:
				put_protobuf_text(config->stream, &options, &messageTypeNAddress, m);
				break;
			default:
				put_debug(config->stream, &messageTypeNAddress, m);
			}
    	}
		else
			LOG(ERROR) << ERR_DECODE_MESSAGE << " " << pkt2utilstring::hexString(buffer, bytes);
    }

    free(buffer);
	if (!config->file_name.empty())
	{
		if (config->stream)
		{
			delete config->stream;
			config->stream = NULL;
		}
	}

	int r = nn_shutdown(accept_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}
	
	close(accept_socket);
	accept_socket = 0;

	if (config->stop_request == 2)
		goto START;

	return r;
}

/**
 *  @brief Stop writer
 *  @param config program config
 *  @return 0- success
 *          >0- config is not initialized yet
 */
int stop
(
	Config *config
)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	config->stop_request = 1;
	return ERR_OK;
}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	return ERR_OK;
}
