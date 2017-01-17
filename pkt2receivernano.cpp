#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <glog/logging.h>

#include "pkt2receivernano.h"
#include "input-packet.h"

/**
  * Return:  0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
  */
int pkt2_receiever_nano(Config *config)
{
    InputPacket packet('T', config->buffer_size);
    /*
    struct addrinfo *res;
    if (get_addr_info(config, &res))
    {
		LOG(ERROR) << "Can not resolve";
		return 1;
    }
    struct sockaddr_storage *svc_addr = packet.get_socket_addr_dst();
    //svc_addr

	int socket = listen_port(config, res);

	// Free the res linked list after we are done with it	
	freeaddrinfo(res);

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

    struct sockaddr_storage *client_addr = packet.get_socket_addr_src();

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
    */
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