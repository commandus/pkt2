#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <glog/logging.h>

#include "tcpreceivernano.h"
#include "input-packet.h"

void *get_in_addr
(
    struct sockaddr *sa
)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr); 
	return &(((struct sockaddr_in6 *)sa)->sin6_addr); 
}

int listen_port(Config *config)
{
	// Before using hint you have to make sure that the data structure is empty 
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
	// Set the attribute for hint
	hints.ai_family = AF_UNSPEC;        // We don't care V4 AF_INET or 6 AF_INET6
	hints.ai_socktype = SOCK_STREAM;    // TCP Socket SOCK_DGRAM 
	hints.ai_flags = AI_PASSIVE; 
	
	// Fill the res data structure and make sure that the results make sense. 
   	struct addrinfo *res;
	int status = getaddrinfo(NULL, uint64ToString(config->port).c_str(), &hints, &res);
	if (status != 0)
	{
        LOG(ERROR) << "getaddrinfo error: " << gai_strerror(status);
        return -1;
	}
	
	// Create Socket and check if error occured afterwards
	int listner = socket(res->ai_family,res->ai_socktype, res->ai_protocol);
	if (listner < 0 )
	{
        LOG(ERROR) << "socket error: " << gai_strerror(status);
        return -1;
	}
	int reuseaddr = 1;
    if (setsockopt(listner, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) ==-1) 
    {
        LOG(ERROR) << "setsockopt error";
        return -1;
    }

	// Bind the socket to the address of my local machine and port number 
	status = bind(listner, res->ai_addr, res->ai_addrlen); 
	if (status < 0)
	{
        LOG(ERROR) << "bind: " << gai_strerror(status);
        return -1;
	}

	status = listen(listner, 10); 
	if (status < 0)
	{
		LOG(ERROR) << "listen: " << gai_strerror(status);
        return -1;
	}
	
	// Free the res linked list after we are done with it	
	freeaddrinfo(res);
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
	int socket = listen_port(config);
	if (socket == -1)
	{
		LOG(ERROR) << "exit on listener error";
		return 1;
	}

    int nano_socket = nn_socket(AF_SP, NN_PUSH);
    if (nn_connect(nano_socket, config->message_url.c_str()) < 0)
    {
        LOG(ERROR) << "Can not connect to the IPC url " << config->message_url;
        close(socket);	
		return 2;
    }


    InputPacket packet(config->buffer_size);
    struct sockaddr_storage *client_addr = packet.get_socket_addr();

    if (packet.error() != 0)
    {
        close(socket);	
		return 3;
    }

    while (!config->stop_request)
    {
		// Wait now for a connection to accept
		// Accept a new connection and return back the socket desciptor 
    	socklen_t addr_size = sizeof(struct sockaddr_storage);
		int new_conn_fd = accept(socket, (struct sockaddr *) client_addr, &addr_size);	
		if (new_conn_fd < 0)
		{
            LOG(ERROR) << "accept error: " << new_conn_fd << ": " << gai_strerror(new_conn_fd);
			continue;
		}

        char s[INET6_ADDRSTRLEN]; // an empty string 
		inet_ntop(client_addr->ss_family, get_in_addr((struct sockaddr *) client_addr), s, sizeof(s)); 
        LOG(INFO) << "connected to: " << s;

        packet.length = read(new_conn_fd, packet.data(), packet.size);
   
        if (packet.length < 0) 
        {
            LOG(ERROR) << "error reading from socket " << packet.length << ": " << gai_strerror(new_conn_fd);
            close(new_conn_fd);
            continue;
        }

        // Send immediate reply if provided
        /*
		int status = send(new_conn_fd, "Welcome", 7, 0);
		if (status == -1)
		{
            LOG(ERROR) << "send error";
			close(new_conn_fd);

            close(socket);	
            nn_shutdown(nano_socket, 0);
			return 4;
		}
        */

		// Close the socket
		close(new_conn_fd);

        // send message to the nano queue

        int bytes = nn_send(nano_socket, packet.get(), packet.length, 0);
        if (bytes != packet.length)
        {
            LOG(ERROR) << "nano message send error, sent " << bytes << " of " << packet.length;
        }
	}
   	close(socket);	
    return nn_shutdown(nano_socket, 0);
}

/**
  * Return 0- success
  *        1- config is not initialized yet
  */
int stop(Config *config)
{
    if (!config)
        return 1;
    config->stop_request = true;
    // wake up

}