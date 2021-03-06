#include <iostream>
#include <string.h>
#include <stdio.h>
#include <nanomsg/bus.h>

#include <glog/logging.h>

#if defined(_WIN32) || defined(_WIN64)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include "platform.h"
#include "tcpreceivernano.h"
#include "errorcodes.h"
#include "input-packet.h"
#include "helper_socket.h"

#include "huffmanmodifieddecoder.h"

int get_addr_info
(
	Config *config,
	struct addrinfo **res
)
{
	// Before using hint you have to make sure that the data structure is empty 
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	// Set the attribute for hint
	hints.ai_family = AF_UNSPEC;        // We don't care V4 AF_INET or 6 AF_INET6
	hints.ai_socktype = SOCK_STREAM;    // TCP Socket SOCK_DGRAM 
	hints.ai_flags = AI_PASSIVE; 
	
	// Fill the res data structure and make sure that the results make sense. 
	int status = getaddrinfo(config->intface.c_str(), pkt2utilstring::toString(config->port).c_str(), &hints, res);
	if (status != 0)
	{
		LOG(ERROR) << ERR_GET_ADDRINFO << gai_strerror(status);
		return ERRCODE_GET_ADDRINFO;
	}
	return ERR_OK;
}

int listen_port
(
	struct addrinfo *res
)
{
	// Create Socket and check if error occured afterwards
	int listner = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listner < 0 )
	{
		LOG(ERROR) << ERR_SOCKET_CREATE << gai_strerror(listner);
		return ERRCODE_SOCKET_CREATE;
	}
	int reuseaddr = 1;
	if (setsockopt(listner, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) ==-1) 
	{
		close(listner);
		LOG(ERROR) << ERR_SOCKET_SET_OPTIONS;
		return ERRCODE_SOCKET_SET_OPTIONS;
	}

	// Bind the socket to the address of my local machine and port number 
	// LOG(INFO) << "Bind len " << res->ai_addrlen;
	int status = bind(listner, res->ai_addr, res->ai_addrlen); 
	if (status < 0)
	{
		close(listner);
		LOG(ERROR) << ERR_SOCKET_BIND << errno << ": " << strerror(errno) << ", addr len " << res->ai_addrlen;
		return ERRCODE_SOCKET_BIND;
	}

	status = listen(listner, 10); 
	if (status < 0)
	{
		close(listner);
		LOG(ERROR) << ERR_SOCKET_LISTEN << errno << ": " << strerror(errno);
		return ERRCODE_SOCKET_LISTEN;
	}
	return listner;
}

/**
* Return:  0- success
*          1- can not listen port
*          2- invalid nano socket URL
*          3- buffer allocation error
*          4- send error, re-open 
*/
int tcp_receiever_nano(Config *config)
{
START:	
	HuffmanModifiedDecoder decoder;
	decoder.setMode(config->compression_type);
	if (!config->frequence_file.empty())
		decoder.setTreeFromFrequenciesFile(config->frequence_file);
	else
		if (!config->codemap_file.empty())
			decoder.setTreeFromCodesFile(config->codemap_file);
	decoder.setEscapeCode(config->escape_code, 8);
	if (!config->eof_code.empty())
		decoder.setEOFCode(config->eof_code);
	decoder.setValidPacketSizes(config->valid_sizes);

	config->stop_request = 0;
	struct addrinfo *addr_dst;
	if (get_addr_info(config, &addr_dst))
	{
		LOG(ERROR) << ERR_GET_ADDRINFO;
		return ERRCODE_GET_ADDRINFO;
	}

	InputPacket packet('T', config->buffer_size);
	packet.set_socket_addr_dst(addr_dst);

	int socket_accept = listen_port(addr_dst);
	// Free the res linked list after we are done with it	
	freeaddrinfo(addr_dst);
	if (socket_accept < 0)
	{
		LOG(ERROR) << ERR_SOCKET_LISTEN 
			<< ((struct sockaddr_in*) addr_dst)->sin_addr.s_addr
			<< ":" << ((struct sockaddr_in*) addr_dst)->sin_port;
		return ERRCODE_SOCKET_LISTEN;
	}

	LOG(INFO) << MSG_START << " socket " << socket_accept;

	int nano_socket = nn_socket(AF_SP, NN_BUS);
	WAIT_CONNECTION(1); // wait for connections
	int timeout = 100;
	int r = nn_setsockopt(nano_socket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout));
	if (r < 0)
	{
		close(socket_accept);
		LOG(ERROR) << ERR_NN_SET_SOCKET_OPTION << config->message_url << " " << errno << ": " << nn_strerror(errno);
		return ERRCODE_NN_SET_SOCKET_OPTION;
	}

	int eoid = nn_connect(nano_socket, config->message_url.c_str());
    if (eoid < 0)
    {
		LOG(ERROR) << ERR_NN_CONNECT << config->message_url << " " << errno << ": " << nn_strerror(errno);
		close(socket_accept);
		return ERRCODE_NN_CONNECT;
    }

	if (packet.error() != 0)
	{
		close(socket_accept);	
		LOG(ERROR) << ERR_GET_ADDRINFO << config->message_url;
		return ERRCODE_NN_CONNECT;
	}

	if (config->verbosity >= 2)
	{
		LOG(INFO) << MSG_NN_BIND_SUCCESS << config->message_url << " with time out: " << timeout;
	}

	struct sockaddr_in *src = packet.get_sockaddr_src();
	socklen_t addr_size = sizeof(struct sockaddr_in);

	while (!config->stop_request)
	{
		// Wait now for a connection to accept, write source IP address into packet
		if (config->verbosity > 1)
			LOG(INFO) << "loop";

		// Accept a new connection and return back the socket descriptor
		int new_conn_fd = accept(socket_accept, (struct sockaddr *) src, &addr_size);
		if (new_conn_fd < 0)
		{
			LOG(ERROR) << ERR_NN_ACCEPT << gai_strerror(errno) << " (interrupted?), continue.";
			continue;
		}
		// Read
		size_t sz = read(new_conn_fd, packet.data(), packet.max_data_size);
		sz = decoder.decode_buffer2buffer(packet.data(), packet.max_data_size, packet.data(), sz, config->compression_offset);
		packet.setLength(sz);

		// Close the socket
		close(new_conn_fd);

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
		SEND_FLUSH(1);	// BUGBUG 0 - nn_send 
		
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
				LOG(INFO) << MSG_NN_SENT_SUCCESS << config->message_url << " data: " << bytes << " bytes: " << pkt2utilstring::hexString(packet.get(), packet.size);
				if (config->verbosity >= 2)
				{
					std::cerr << MSG_NN_SENT_SUCCESS << config->message_url << " data: " << bytes << " bytes: " << pkt2utilstring::hexString(packet.get(), packet.size)
					<< " payload: " << pkt2utilstring::hexString(packet.data(), packet.length) << " bytes: " << packet.length
					<< std::endl;
				}
			}
		}
	}

	if (socket_accept >= 0)
	{
		if (config->verbosity > 1)
			LOG(INFO) << "close accept socket";
		close(socket_accept);	
		socket_accept = 0;
	}
	r = nn_shutdown(nano_socket, eoid);
	
	if (nano_socket)
	{
		if (config->verbosity > 1)
			LOG(INFO) << "close nano socket";
		close(nano_socket);	
		nano_socket = 0;
	}
	
	LOG(ERROR) << MSG_STOP;
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
