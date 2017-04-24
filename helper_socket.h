#ifndef HELPER_SOCKET_H
#define HELPER_SOCKET_H 1

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <iostream>
#include <string>
#include "errorcodes.h"

/**
 * @brief Return struct addrinfo
 * @param host host name or address
 * @param port port number
 * @param res return value
 */
int get_addr_info
(
	const std::string &host,
	int port,
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
	int status = getaddrinfo(host.c_str(), toString(port).c_str(), &hints, res);
	if (status != 0)
	{
		std::cerr << ERR_GET_ADDRINFO << gai_strerror(status);
		return ERRCODE_GET_ADDRINFO;
	}
    return ERR_OK;
}

/**
 * @brief open TCP/IP socket
 * @param sock return value
 * @param host host name or address
 * @param port port number
 */
int open_socket
(
	int &sock,
	const std::string &host,
	int port
)
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		std::cerr << ERR_SOCKET_CREATE << strerror(errno) << std::endl;
		return ERRCODE_SOCKET_CREATE;
	}

	int optval = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	struct addrinfo *res;
	if (get_addr_info(host, port, &res))
	{
		std::cerr << ERR_GET_ADDRINFO;
		return ERRCODE_GET_ADDRINFO;
	}

	if (connect(sock, (struct sockaddr *) res->ai_addr, res->ai_addrlen) < 0)
	{
		std::cerr << ERR_SOCKET_CONNECT << host << ":" << port << ". " << strerror(errno) << std::endl;
		freeaddrinfo(res);
		shutdown(sock, SHUT_RDWR);
		close(sock);
		return ERRCODE_SOCKET_CONNECT;
	}

	// Free the res linked list after we are done with it
	freeaddrinfo(res);

	return ERR_OK;
}

#endif
