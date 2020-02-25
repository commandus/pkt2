#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <glog/logging.h>

#include "platform.h"
#include "pbjson.hpp"

#include "pkt2js-impl.h"
#include "input-packet.h"
#include "errorcodes.h"
#include "utilprotobuf.h"
#include "utilstring.h"
#include "packet2message.h"

using namespace google::protobuf;
using namespace google::protobuf::io;

/**
 * @brief Send message to the control NN_BUS
 */
void control_message
(
	Config *config,
	int fd,
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
	write(fd, s.c_str(), s.size());
}

/**
  * @return: 0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
  */
int pkt2js(Config *config)
{
	config->session_id = 0;
	config->count_packet_in = 0;
	config->count_packet_out = 0;
	START:
	config->stop_request = 0;

	// IN socket
	FILE *fin;
	if (config->in_url.empty()) {
		fin = stdin;
	} else {
		fin = fopen(config->in_url.c_str(), "r");
	}
    int eid = fileno(fin);
    if (eid < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->in_url << " " << errno << ": " << strerror(errno);;
		return ERRCODE_NN_BIND;
    }

	// OUT socket
	FILE *fout;
	if (config->out_url.empty()) {
		fout = stdout;
	} else {
		fout = fopen(config->in_url.c_str(), "r");
	}
    int eoid = fileno(fout);
    if (eoid < 0) {
		LOG(ERROR) << ERR_NN_SOCKET << config->out_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_BIND;
    }

	ProtobufDeclarations declarations(config->proto_path, config->verbosity);
	Pkt2OptionsCache options_cache(&declarations);
	Packet2Message packet2Message(&declarations, &options_cache, config->verbosity);

	char *buf = (char *) malloc(config->buffer_size);
	while (!config->stop_request)
	{
		config->session_id++;
		int bytes = read(eid, buf, config->buffer_size);

		// calculate payload size (substract address headers)
		int payload_size = InputPacket::getPayloadSize(bytes);
		
		config->count_packet_in++;
		
		if (payload_size < 0) {
			if (errno == EINTR) {
				LOG(ERROR) << ERR_INTERRUPTED;
				config->stop_request = true;
				break;
			}
			else
				LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
			continue;
		}
		else
		{
			if (payload_size == 0) {
				config->count_packet_out++;
				LOG(INFO) << MSG_EMPTY_PACKET;
				continue;
			}
		}
		
		if (config->allowed_packet_sizes.size())
		{
			if (std::find(config->allowed_packet_sizes.begin(), config->allowed_packet_sizes.end(), payload_size) == config->allowed_packet_sizes.end()) {
				LOG(INFO) << MSG_PACKET_REJECTED << payload_size;
				continue;
			}
		}
		if (buf) {
			InputPacket packet(buf, bytes);
			
			if (config->verbosity > 1)
				LOG(INFO) << MSG_PACKET_HEX << hexString(std::string((const char *) packet.data(), (size_t) packet.length)) << std::endl;

			if (packet.error() != 0) {
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

			std::string outstr;
			switch (config->mode) {
				case 1:	// hex
					outstr = hexString(stringDelimitedMessage(&messageTypeNAddress, *m));
					break;
				case 2:	// binary
					outstr = stringDelimitedMessage(&messageTypeNAddress, *m);
					break;
				default:	// hex
					pbjson::pb2json(m, outstr);
					break;
			}
			
			int sent = write(eoid, outstr.c_str(), outstr.size());

			if (sent < 0) {
				LOG(ERROR) << ERR_NN_SEND << sent;
			}
			else {
				config->count_packet_out++;
				if (config->verbosity >= 1) {
					std::string s;
					pbjson::pb2json(m, s);
					LOG(INFO) << MSG_SENT << sent << " " 
						<< hexString(outstr) 
						<< std::endl 
						<< s;
				}
			}
        }
	}

	free(buf);

	if (eoid) {
		fclose(fout);
		fout = NULL;
		eoid = 0;
	}

	if (eid) {
		fclose(fin);
		fin = NULL;
		eid = 0;
	}

	if (config->stop_request == 2)
		goto START;

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
