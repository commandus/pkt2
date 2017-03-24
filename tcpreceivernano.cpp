#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <glog/logging.h>

#include "tcpreceivernano.h"
#include "errorcodes.h"
#include "input-packet.h"

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
	int status = getaddrinfo(config->intface.c_str(), toString(config->port).c_str(), &hints, res);
	if (status != 0)
	{
        LOG(ERROR) << ERR_GET_ADDRINFO << gai_strerror(status);
        return ERRCODE_GET_ADDRINFO;
	}
    return ERR_OK;
}

int listen_port(
    Config *config,
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
        LOG(ERROR) << ERR_SOCKET_SET_OPTIONS;
        return ERRCODE_SOCKET_SET_OPTIONS;
    }

	// Bind the socket to the address of my local machine and port number 
	int status = bind(listner, res->ai_addr, res->ai_addrlen); 
	if (status < 0)
	{
        LOG(ERROR) << ERR_SOCKET_BIND << gai_strerror(status);
        return ERRCODE_SOCKET_BIND;
	}

	status = listen(listner, 10); 
	if (status < 0)
	{
		LOG(ERROR) << ERR_SOCKET_LISTEN << gai_strerror(status);
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
    struct addrinfo *res;
    if (get_addr_info(config, &res))
    {
		LOG(ERROR) << ERR_GET_ADDRINFO;
		return ERRCODE_GET_ADDRINFO;
    }

    InputPacket packet('T', config->buffer_size);
    packet.set_socket_addr_dst(res);

	int socket = listen_port(config, res);

	// Free the res linked list after we are done with it	
	freeaddrinfo(res);

	if (socket == -1)
	{
		LOG(ERROR) << ERR_SOCKET_LISTEN;
		return ERRCODE_SOCKET_LISTEN;
	}

    int nano_socket = nn_socket(AF_SP, NN_BUS);
    sleep (1); // wait for connections
	int timeout = 100;
	nn_setsockopt(nano_socket, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout));

    if (nn_bind(nano_socket, config->message_url.c_str()) < 0)
    {
        LOG(ERROR) << ERR_NN_CONNECT << config->message_url;
        close(socket);	
		return ERRCODE_NN_CONNECT;
    }

    if (packet.error() != 0)
    {
        close(socket);	
        LOG(ERROR) << ERR_GET_ADDRINFO << config->message_url;
		return ERRCODE_NN_CONNECT;
    }

    while (!config->stop_request)
    {
		// Wait now for a connection to accept
    	struct addrinfo *res;
        struct sockaddr_in *src = packet.get_sockaddr_src();
		socklen_t addr_size = sizeof(struct sockaddr_in);

		// Accept a new connection and return back the socket descriptor
		int new_conn_fd = accept(socket, (struct sockaddr *) src, &addr_size);
		if (new_conn_fd < 0)
		{
            LOG(ERROR) << ERR_NN_ACCEPT << gai_strerror(errno);
			continue;
		}

		// Read
        packet.length = read(new_conn_fd, packet.data(), packet.size);
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
        			inet_ntoa(packet.get_sockaddr_dst()->sin_addr) << ":" << ntohs(packet.get_sockaddr_dst()->sin_port) << " " << packet.length << " bytes.";
    			if (config->verbosity >= 2)
    			{
    				std::cerr << inet_ntoa(src->sin_addr) << ":" << ntohs(src->sin_port) << "->" <<
    						inet_ntoa(packet.get_sockaddr_dst()->sin_addr) << ":" << ntohs(packet.get_sockaddr_dst()->sin_port) << " "
							<< packet.length << " bytes: " << hexString(packet.data(), packet.length)
							<< std::endl;
    			}
    		}
        }
        // send message to the nano queue

        int bytes = nn_send(nano_socket, packet.get(), packet.length, 0);
        if (bytes != packet.length)
        {
            LOG(ERROR) << ERR_NN_SEND << bytes << " of " << packet.length;
        }
	}
   	close(socket);	
    return nn_shutdown(nano_socket, 0);
}

/**
 * @param config
 * @return 0- success
  *        1- config is not initialized yet
 */
int stop(Config *config)
{
    if (!config)
        return ERRCODE_NO_CONFIG;
    config->stop_request = true;
    // wake up
    return ERR_OK;

}
