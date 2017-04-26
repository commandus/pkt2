/**
 * Read message types and data as json line per line.
 * Line format is:
 *   Packet.MessageType:{"json-object-in-one-line"}
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <glog/logging.h>

#include "tcpemitterline.h"
#include "utilstring.h"
#include "tcpemitter-config.h"
#include "errorcodes.h"

/**
 * @brief send loop
 * @param config Configuration
 * @return 0- OK
 */
int tcp_emitter_line
(
	Config *config
)
{
START:
	config->stop_request = 0;

	struct hostent *server = gethostbyname(config->intface.c_str());

	if (server == NULL)
	{
		LOG(ERROR) << ERR_GET_ADDRINFO << config->intface;
		return ERRCODE_GET_ADDRINFO;
	}

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(config->port);

	// open text file or read from stdin
	std::istream *strm;
	if (!config->file_name.empty())
		strm = new std::ifstream(config->file_name.c_str());
	else
		strm = &std::cin;

	// read lines from the file or stdin
	while ((!config->stop_request) && (!strm->eof()))
	{
		// send message to the nano queue
		std::string line;
		*strm >> line;
		line = hex2string(line);
		if (line.size())
		{
			for (int c = 0; c < config->count; c++)
			{
				int sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0)
				{
					LOG(ERROR) << ERR_SOCKET_CREATE;
					return ERRCODE_SOCKET_CREATE;
				}

				if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
				{
					LOG(ERROR) << ERR_SOCKET_CONNECT << config->intface;
					close(sock);
					return ERRCODE_SOCKET_CONNECT;
				}
				int n = write(sock, line.c_str(), line.size());
				close(sock);

				if (n < 0)
					LOG(ERROR) << ERR_SOCKET_SEND << config->intface << " " << n;
				if (config->stop_request)
					break;
				sleep(config->delay);
			}
		}
	}
	if (!config->file_name.empty())
		delete strm;

	
	if (config->stop_request == 2)
		goto START;
	return ERR_OK;
}

/**
 * @brief stop
 * @param config Configuration
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
