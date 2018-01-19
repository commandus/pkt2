#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <nanomsg/bus.h>
#include <fstream>
#include <sstream>

#include <glog/logging.h>

#include "platform.h"
#include "freceiver-impl.h"
#include "errorcodes.h"
#include "input-packet.h"
#include "helper_socket.h"

#include "messageformat.h"

static int char2int
(
	char input
)
{
	if (input >= '0' && input <= '9')
		return input - '0';
	if (input >= 'A' && input <= 'F')
		return input - 'A' + 10;
	if (input >= 'a' && input <= 'f')
		return input - 'a' + 10;
	return 0;
}

/**
 * @brief copy bytes from input to output stream
 * @return -1 EOF, copied bytes count otherwise 
 */
int readData
(
	std::ostream &ostrm,
	int cmd,
	int packet_size,
	std::istream *istrm
)
{
	if (istrm->bad()) 
		return -1;	// EOF or error
	int r = 0;
	switch(cmd)
	{
		case FILE_MODE_TEXT_HEX:
			{
				std::string line;
				if (std::getline(*istrm, line))
				{
					r = line.size() / 2;
					if ((packet_size > 0) && (packet_size < r))
						r = packet_size;
					for (int i = 0; i < r; i++)
					{
						unsigned char v = char2int(line.at(i * 2)) * 16 + char2int(line.at((i * 2) + 1));
						ostrm.write((char *)&v, 1);
					}
				}	
			}
			break;
		case FILE_MODE_TEXT_INT:
			{
				std::string line;
				while (std::getline(*istrm, line))
				{
					int v = strtol(line.c_str(), NULL, 10);
					r++;
					if ((packet_size > 0) && (r > packet_size))
						break;
					ostrm.write((char *)&v, 1);	// last byte
				}	
			}
			break;
		default:	// case FILE_MODE_BIN:
		{
			char v;
			while (istrm->read(&v, 1))
			{
				ostrm.write(&v, 1);
				r++;
				if ((packet_size > 0) && (r >= packet_size))
					break;
			}
		}
	}
	return r;
}

/**
* Return:  0- success
*          1- can not listen port
*          2- invalid nano socket URL
*          3- buffer allocation error
*          4- send error, re-open 
*/
static int fileReceieve(Config *config)
{
START:	
	config->stop_request = 0;
	InputPacket packet('T', config->buffer_size);

	LOG(INFO) << MSG_START << " file " << config->filename_in;
	
	std::istream *f;
	if (config->filename_in.empty())
		f = &std::cin;
	else
	{
		if (config->file_mode == 0)
			f = new std::fstream(config->filename_in.c_str(), std::ios::in | std::ios::binary);
		else
			f = new std::fstream(config->filename_in.c_str(), std::ios::in);
	}

	int nano_socket = nn_socket(AF_SP, NN_BUS);
	WAIT_CONNECTION(1);
	int timeout = 100;
	int r = nn_setsockopt(nano_socket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout));
	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SET_SOCKET_OPTION << config->message_url << " " << errno << ": " << nn_strerror(errno);
		if (config->file_mode == 0)
			delete f;
		return ERRCODE_NN_SET_SOCKET_OPTION;
	}

	int eoid = nn_connect(nano_socket, config->message_url.c_str());
    if (eoid < 0)
    {
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_CONNECT;
    }

	if (packet.error() != 0)
	{
		if (config->file_mode == 0)
			delete f;
		LOG(ERROR) << ERR_GET_ADDRINFO << config->message_url;
		return ERRCODE_NN_CONNECT;
	}

	if (config->verbosity >= 2)
	{
		LOG(INFO) << MSG_NN_BIND_SUCCESS << config->message_url << " with time out: " << timeout;
	}

	struct sockaddr_in *src = packet.get_sockaddr_src();

	while (!config->stop_request)
	{
		// Wait now for a connection to accept, write source IP address into packet
		if (config->verbosity > 1)
			LOG(INFO) << "loop";

		std::stringstream ss;
		if (readData(ss, config->file_mode, config->packet_size, f) == 0)
			break;
		std:: string s = ss.str();
		size_t sz = s.size();
		
		// Read
		packet.setLength(sz);
		memcpy(packet.data(), s.c_str(), sz);
		if (packet.length <= 0)
		{
			LOG(ERROR) << ERR_SOCKET_READ << gai_strerror(errno);
			continue;
		}
		else
		{
			if (config->verbosity >= 1)
			{
				LOG(INFO) << inet_ntoa(src->sin_addr) << ":" << ntohs(src->sin_port) << "->" <<
					inet_ntoa(packet.get_sockaddr_dst()->sin_addr) << ":" << ntohs(packet.get_sockaddr_dst()->sin_port)
					<< " " << packet.length << " bytes.";
				if (config->verbosity >= 2)
				{
					std::cerr << inet_ntoa(src->sin_addr) << ":" << ntohs(src->sin_port) << "->" <<
							inet_ntoa(packet.get_sockaddr_dst()->sin_addr) << ":" << ntohs(packet.get_sockaddr_dst()->sin_port) << " "
							<< packet.size
							<< std::endl;
				}
			}
		}

		// send message to the nano queue
		int bytes = nn_send(nano_socket, packet.get(), packet.size, 0);
		// flush
		SEND_FLUSH(100);	// BUGBUG 0 - nn_send 

		if (bytes != packet.size)
		{
			if (bytes < 0)
				LOG(ERROR) << ERR_NN_SEND << " " << errno << ": " << nn_strerror(errno);
			else
				LOG(ERROR) << ERR_NN_SEND << bytes << ",  payload " << packet.length;
		}
		else
		{
			if (config->verbosity >= 1)
			{
				LOG(INFO) << MSG_NN_SENT_SUCCESS << config->message_url << " data: " << bytes << " bytes: " << hexString(packet.get(), packet.size);
				if (config->verbosity >= 2)
				{
					std::cerr << MSG_NN_SENT_SUCCESS << config->message_url << " data: " << bytes << " bytes: " << hexString(packet.get(), packet.size)
					<< " payload: " << hexString(packet.data(), packet.length) << " bytes: " << packet.length
					<< std::endl;
				}
			}
		}
	}

	if (!config->filename_in.empty())
		delete f;

	r = nn_shutdown(nano_socket, eoid);

	if (nano_socket)
	{
		close(nano_socket);	
		nano_socket = 0;
	}

	LOG(INFO) << MSG_STOP;
	if (config->stop_request == 2)
		goto START;

	if (r < 0)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_SHUTDOWN;
	}
	return ERR_OK;
}

/**
* Return:  0- success
*          1- can not listen port
*          2- invalid nano socket URL
*          3- buffer allocation error
*          4- send error, re-open 
*/
static int filePrint(Config *config)
{
	config->stop_request = 0;
	
	ProtobufDeclarations declarations(config->proto_path, config->verbosity);
	Pkt2OptionsCache options_cache(&declarations);
	Packet2Message packet2Message(&declarations, &options_cache, config->verbosity);

	InputPacket packet('T', config->buffer_size);
	if (packet.error() != 0)
	{
		LOG(ERROR) << ERR_GET_ADDRINFO << config->message_url;
		return ERRCODE_NN_CONNECT;
	}
	
	std::istream *f;
	if (config->filename_in.empty())
	{
		f = &std::cin;
	}
	else
	{
		if (config->file_mode == 0)
			f = new std::ifstream(config->filename_in.c_str(), std::ifstream::in | std::ifstream::binary);
		else
			f = new std::ifstream(config->filename_in.c_str(), std::fstream::in);
	}


	struct sockaddr_in *src = packet.get_sockaddr_src();

	while (!config->stop_request)
	{
		std::stringstream ss;
		if (readData(ss, config->file_mode, config->packet_size, f) <= 0)
			break;
		std:: string s = ss.str();
		size_t sz = s.size();

		if (sz < config->packet_size)
			break;
		if (sz > config->buffer_size)
		{
			LOG(ERROR) << ERR_TOO_SMALL << config->buffer_size << " for " << sz << " bytes";
			continue;
		}
		
		// Read
		packet.setLength(sz);
		memcpy(packet.data(), s.c_str(), sz);
		if (packet.length <= 0)
		{
			LOG(ERROR) << ERR_SOCKET_READ << gai_strerror(errno);
			continue;
		}
		if (packet.error() != 0)
		{
			LOG(ERROR) << ERR_PACKET_PARSE << packet.error();
			continue;
		}

		// try to parse and print message
		PacketParseEnvironment packet_env(
			(sockaddr *) packet.get_sockaddr_src(),
			(sockaddr *) packet.get_sockaddr_dst(),
			std::string((const char *) packet.data(), (size_t) packet.length),
			&options_cache,
			""
		);
		google::protobuf::Message *m = packet2Message.parsePacket(&packet_env);
		if (!m)
		{
			LOG(ERROR) << ERR_PACKET_PARSE;
			continue;
		}

		MessageTypeNAddress messageTypeNAddress(m->GetTypeName());
		switch (config->mode)
		{
			case MODE_JSON:
				put_json(&std::cout, &options_cache, &messageTypeNAddress, m);
				break;
			case MODE_CSV:
				put_csv(&std::cout, &options_cache, &messageTypeNAddress, m);
				break;
			case MODE_TAB:
				put_tab(&std::cout, &options_cache, &messageTypeNAddress, m);
				break;
			case MODE_SQL:
				put_sql(&std::cout, &options_cache, &messageTypeNAddress, m);
				break;
			case MODE_SQL2:
				put_sql2(&std::cout, &options_cache, &messageTypeNAddress, m);
				break;
			case MODE_PB_TEXT:
				put_protobuf_text(&std::cout, &options_cache, &messageTypeNAddress, m);
				break;
			case MODE_PRINT_DBG:
				put_debug(&std::cout, &messageTypeNAddress, m);
				break;
			default:
				put_debug(&std::cout, &messageTypeNAddress, m);
		}

		delete m;
	}

	if (!config->filename_in.empty())
		delete f;

	return ERR_OK;
}

/**
* Return:  0- success
*          1- can not listen port
*          2- invalid nano socket URL
*          3- buffer allocation error
*          4- send error, re-open 
*/
int file_receiever(Config *config)
{
	if (config->mode >= 0)
		filePrint(config);
	else
		fileReceieve(config);
}

/**
* @param config configuration
* @return 0- success
*        1- config is not initialized yet
*/
int stop(Config *config)
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
