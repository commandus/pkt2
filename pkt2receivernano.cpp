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

#define CONTROL_TYP_RECEIVED_CODE		1
#define CONTROL_TYP_RECEIVED			"packet received"
#define CONTROL_TYP_ERROR_CODE			2
#define CONTROL_TYP_ERROR				"message not sent"
#define CONTROL_TYP_MSG_SENT_CODE		3
#define CONTROL_TYP_MSG_SENT			"message sent"
#define CONTROL_TYP_EMPTY_CODE			4
#define CONTROL_TYP_EMPTY				"packet is empty"

/**
 * @brief Send message to the control NN_BUS
 */
void control_message
(
	Config *config,
	int typ,
	int sz,
	const std::string &msg,
	const std::string &payload
)
{
	std::stringstream ss;
	ss << config->session_id << "\t"
		<< config->count_packet_in << "\t" 
		<< config->count_packet_out << "\t" 
		<< typ << "\t"
		<< sz << "\t" 
		<< msg << "\t" 
		<< payload << std::endl; 
	std::string s(ss.str());
	nn_send(config->socket_control, s.c_str(), s.size(), 0);
}

/**
  * @return: 0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
  */
int pkt2_receiever_nano(Config *config)
{
	config->session_id = 0;
	config->count_packet_in = 0;
	config->count_packet_out = 0;
	START:
	config->stop_request = 0;

	// Control socket
    config->socket_control = nn_socket(AF_SP, NN_BUS);
	if (config->socket_control < 0)
	{
			LOG(ERROR) << ERR_NN_SOCKET << config->control_url << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_SOCKET;
	}
	int ecid = nn_bind(config->socket_accept, config->control_url.c_str());
    if (ecid < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->control_url << " " << errno << ": " << nn_strerror(errno);;
		return ERRCODE_NN_BIND;
    }

	// IN socket
    config->socket_accept = nn_socket(AF_SP, NN_BUS); // was NN_SUB
    // int r = nn_setsockopt(config->socket_accept, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (config->socket_accept < 0)
	{
			LOG(ERROR) << ERR_NN_SOCKET << config->in_url << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_SOCKET;
	}

	// bind
	int eid = nn_bind(config->socket_accept, config->in_url.c_str());
    if (eid < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->in_url << " " << errno << ": " << nn_strerror(errno);;
		return ERRCODE_NN_BIND;
    }

	// OUT socket
    int nn_sock_out = nn_socket(AF_SP, NN_BUS); // was NN_SUB
    // int r = nn_setsockopt(config->socket_accept, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
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
		config->session_id++;
		char *buf = NULL;
		int bytes = nn_recv(config->socket_accept, &buf, NN_MSG, 0);

		int payload_size = InputPacket::getPayloadSize(bytes);
		
		control_message(config, CONTROL_TYP_RECEIVED_CODE, payload_size, CONTROL_TYP_RECEIVED, "");
		config->count_packet_in++;
		
		if (payload_size < 0)
		{
			control_message(config, CONTROL_TYP_ERROR_CODE, payload_size, CONTROL_TYP_ERROR, ERR_NN_RECV);
			LOG(ERROR) << ERR_NN_RECV << " too small " << payload_size;
			continue;
		}
		else
		{
			if (payload_size == 0)
			{
				config->count_packet_out++;
				control_message(config, CONTROL_TYP_EMPTY_CODE, payload_size, CONTROL_TYP_EMPTY, "");
				LOG(INFO) << MSG_EMPTY_PACKET;
				continue;
			}
		}
		
		if (config->allowed_packet_sizes.size())
		{
			if (std::find(config->allowed_packet_sizes.begin(), config->allowed_packet_sizes.end(), payload_size) == config->allowed_packet_sizes.end())
			{
				LOG(INFO) << MSG_PACKET_REJECTED << payload_size;
				control_message(config, CONTROL_TYP_ERROR_CODE, payload_size, CONTROL_TYP_ERROR, "");
				continue;
			}
			
		}
		if (buf)
		{
			InputPacket packet(buf, bytes);

			if (packet.error() != 0)
			{
				control_message(config, CONTROL_TYP_ERROR_CODE, packet.error(), CONTROL_TYP_ERROR, "");
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
				control_message(config, CONTROL_TYP_ERROR_CODE, 0, CONTROL_TYP_ERROR, "");
				LOG(ERROR) << ERR_PARSE_PACKET << hexString(std::string((const char *) packet.data(), (size_t) packet.length)) << std::endl;
        		continue;
			}
			// send message
			MessageTypeNAddress messageTypeNAddress(m->GetTypeName());
			std::string outstr = stringDelimitedMessage(&messageTypeNAddress, *m);
			int sent = nn_send(nn_sock_out, outstr.c_str(), outstr.size(), 0);
			if (sent < 0)
			{
				control_message(config, CONTROL_TYP_ERROR_CODE, 0, CONTROL_TYP_ERROR, "");
				LOG(ERROR) << ERR_NN_SEND << sent;
			}
			else
			{
				config->count_packet_out++;
				if (config->verbosity >= 1)
				{
					std::string s;
					pbjson::pb2json(m, s);
					LOG(INFO) << MSG_SENT << sent << " " << hexString(outstr) << std::endl 
						<< s;
					control_message(config, CONTROL_TYP_MSG_SENT_CODE, outstr.size(), CONTROL_TYP_MSG_SENT, s);	
					if (config->verbosity >= 2)
					{
						std::cerr << MSG_SENT << sent << " " << hexString(outstr) << std::endl
							<< s << std::endl;
					}
				}
				else
				{
					control_message(config, CONTROL_TYP_MSG_SENT_CODE, outstr.size(), CONTROL_TYP_MSG_SENT, "");
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
    r = r | nn_shutdown(config->socket_accept, eid);
    r = r | nn_shutdown(config->socket_control, ecid);

	if (config->stop_request == 2)
		goto START;

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
	config->stop_request = 1;
	close(config->socket_accept);
	config->socket_accept = 0;
    return ERR_OK;

}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	close(config->socket_accept);
	config->socket_accept = 0;
	return ERR_OK;
}
