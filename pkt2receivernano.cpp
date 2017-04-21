#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include <glog/logging.h>

#include "pbjson.hpp"

#include "pkt2receivernano.h"
#include "input-packet.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "utilstring.h"
#include "packet2message.h"

using namespace google::protobuf;
using namespace google::protobuf::io;

/**
  * @return: 0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
  */
int pkt2_receiever_nano(Config *config)
{
	// IN socket
    int nn_sock_in = nn_socket(AF_SP, NN_BUS); // was NN_SUB
    // int r = nn_setsockopt(nn_sock_in, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (nn_sock_in < 0)
	{
			LOG(ERROR) << ERR_NN_SOCKET << config->in_url << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_SOCKET;
	}

	int eid = nn_connect(nn_sock_in, config->in_url.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << ERR_NN_SOCKET << config->in_url << " " << errno << ": " << nn_strerror(errno);;
		return ERRCODE_NN_SOCKET;
    }

	// OUT socket
    int nn_sock_out = nn_socket(AF_SP, NN_BUS); // was NN_SUB
    // int r = nn_setsockopt(nn_sock_in, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (nn_sock_out < 0)
	{
			LOG(ERROR) << ERR_NN_SOCKET << config->out_url << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_SOCKET;
	}

	int eoid = nn_bind(nn_sock_out, config->out_url.c_str());
    if (eoid < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->out_url << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_BIND;
    }

    ProtobufDeclarations declarations(config->proto_path, config->verbosity);
    Pkt2OptionsCache options_cache(&declarations);
    Packet2Message packet2Message(&declarations, &options_cache, config->verbosity);

    while (!config->stop_request)
    {
        char *buf = NULL;
        int bytes = nn_recv(nn_sock_in, &buf, NN_MSG, 0);

		int payload_size = InputPacket::getPayloadSize(bytes);
        if (payload_size < 0)
        {
        	LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
            continue;
        }
        if (config->allowed_packet_sizes.size())
		{
			if (std::find(config->allowed_packet_sizes.begin(), config->allowed_packet_sizes.end(), payload_size) == config->allowed_packet_sizes.end())
			{
				LOG(INFO) << MSG_PACKET_REJECTED << payload_size;
				continue;
			}
			
		}
        if (buf)
        {
            InputPacket packet(buf, bytes);

            if (packet.error() != 0)
            {
                LOG(ERROR) << ERRCODE_PACKET_PARSE << packet.error();
                continue;
            }

            // packet -> message
            PacketParseEnvironment packet_env(
            		(sockaddr *) packet.get_sockaddr_src(),
					(sockaddr *) packet.get_sockaddr_dst(),
					std::string((const char *) packet.data(), (size_t) packet.length),
					&options_cache,
					config->force_message
			);
            google::protobuf::Message *m = packet2Message.parsePacket(&packet_env);
            if (m == NULL)
            {
        		LOG(ERROR) << ERR_PARSE_PACKET << hexString(std::string((const char *) packet.data(), (size_t) packet.length)) << std::endl;
        		continue;
            }
            // send message
			MessageTypeNAddress messageTypeNAddress(m->GetTypeName());
			std::string outstr = stringDelimitedMessage(&messageTypeNAddress, *m);
			int sent = nn_send(nn_sock_out, outstr.c_str(), outstr.size(), 0);
			if (sent < 0)
				LOG(ERROR) << ERR_NN_SEND << sent;
			else
			{
				if (config->verbosity >= 1)
				{
					std::string s;
					pbjson::pb2json(m, s);
					LOG(INFO) << MSG_SENT << sent << " " << hexString(outstr) << std::endl 
						<< s;
					if (config->verbosity >= 2)
					{
						std::cerr << MSG_SENT << sent << " " << hexString(outstr) << std::endl
						<< s << std::endl;
					}
				}
			}

            if (nn_freemsg(buf))
			{
            	LOG(ERROR) << ERR_NN_FREE_MSG << " " << errno << ": " << nn_strerror(errno);
			}
			sleep(0);	// BUGBUG Pass 0 for https://github.com/nanomsg/nanomsg/issues/182

        }
	}

    int r = nn_shutdown(nn_sock_out, eoid);
    r = r | nn_shutdown(nn_sock_in, eid);

    if (r)
    {
    	LOG(ERROR) << ERR_NN_SHUTDOWN << " " << errno << ": " << nn_strerror(errno);
    	return ERRCODE_NN_SHUTDOWN;
    }
    else
    	return ERR_OK;
}

/**
  * @return 0- success
  *         ERRCODE_STOP- config is not initialized yet
  */
int stop(Config *config)
{
    if (!config)
        return ERRCODE_STOP;
    config->stop_request = true;
    return ERR_OK;
    // wake up

}
