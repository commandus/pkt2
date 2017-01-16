#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <iostream>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>


#include "platform.h"
#include "tcpreceiver-config.h"

int node1 (
    const char *url,
    const char *msg)
{
    int sz_msg = strlen (msg) + 1; // '\0' too
    int sock = nn_socket (AF_SP, NN_PUSH);
    assert (sock >= 0);
    assert (nn_connect (sock, url) >= 0);
    printf ("NODE1: SENDING \"%s\"\n", msg);
    int bytes = nn_send (sock, msg, sz_msg, 0);
    assert (bytes == sz_msg);
    return nn_shutdown (sock, 0);
}

void *get_in_addr
(
    struct sockaddr *sa
)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr); 
	return &(((struct sockaddr_in6 *)sa)->sin6_addr); 
}

int main
(
    int argc, 
    char * argv[]
)
{
	Config config(argc, argv);
    if (config.error() != 0)
		exit(config.error());

	// Before using hint you have to make sure that the data structure is empty 
    struct addrinfo hints;
	memset(& hints, 0, sizeof hints);
	// Set the attribute for hint
	hints.ai_family = AF_UNSPEC;        // We don't care V4 AF_INET or 6 AF_INET6
	hints.ai_socktype = SOCK_STREAM;    // TCP Socket SOCK_DGRAM 
	hints.ai_flags = AI_PASSIVE; 
	
	// Fill the res data structure and make sure that the results make sense. 
   	struct addrinfo *res;
	int status = getaddrinfo(NULL, "8888" , &hints, &res);
	if (status != 0)
	{
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;        
	}
	
	// Create Socket and check if error occured afterwards
	int listner = socket(res->ai_family,res->ai_socktype, res->ai_protocol);
	if(listner < 0 )
	{
        std::cerr << "socket error: " << gai_strerror(status) << std::endl;
	}
	
	// Bind the socket to the address of my local machine and port number 
	status = bind(listner, res->ai_addr, res->ai_addrlen); 
	if (status < 0)
	{
        std::cerr << "bind: " << gai_strerror(status) << std::endl;
	}

	status = listen(listner, 10); 
	if (status < 0)
	{
		std::cerr << "listen: " << gai_strerror(status) << std::endl;
	}
	
	// Free the res linked list after we are done with it	
	freeaddrinfo(res);

	// We should wait now for a connection to accept
	int new_conn_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	char s[INET6_ADDRSTRLEN]; // an empty string 
		
	// Calculate the size of the data structure	
	addr_size = sizeof client_addr;
	
	std::cout << "accepting connections" << std::endl;        	

	while (1)
    {
		// Accept a new connection and return back the socket desciptor 
		new_conn_fd = accept(listner, (struct sockaddr *) & client_addr, &addr_size);	
		if(new_conn_fd < 0)
		{
			fprintf(stderr,"accept: %s\n",gai_strerror(new_conn_fd));
			continue;
		}
	
		inet_ntop(client_addr.ss_family, 
			get_in_addr((struct sockaddr *) &client_addr), s, sizeof(s)); 
		std::cout << "connected to " << s << std::endl;        	
		status = send(new_conn_fd,"Welcome", 7,0);
		if (status == -1)
		{
			close(new_conn_fd);
			exit(4);
		}
		
	}
	// Close the socket before we finish 
	close(new_conn_fd);	
	
	return 0;
}