/**
 * Read message types and data as json line per line.
 * Line format is:
 *   Packet.MessageType:{"json-object-in-one-line"}
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>

#include <glog/logging.h>
#include <google/protobuf/message.h>

#if defined(_WIN32) || defined(_WIN64)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include "messageemitterline.h"
#include "messageemitter-config.h"
#include "errorcodes.h"
#include "protobuf-declarations.h"
#include "pkt2optionscache.h"

size_t parseLine
(
    ProtobufDeclarations *pd,
    Pkt2OptionsCache *poc,
	const std::string &line,
	void **buffer
)
{
    size_t start_pos = line.find(":");
    if (start_pos == std::string::npos)
    {
    	LOG(ERROR) << ERR_PARSE_LINE << line << "Packet.MessageType:{\"json-object-in-one-line\"}";
    	return 0;
    }
	std::string message_type(line.substr(0, start_pos));
	std::string json(line.substr(start_pos));

	google::protobuf::Message *m = pd->decode(message_type, json);
	if (!m)
	{
		LOG(ERROR) << ERR_PARSE_LINE << line;
		return 0;
	}

	return pd->encode(buffer, m);
}

/**
 * send loop
 * @param config
 * @return
 */
int tcp_emitter_line(Config *config)
{
START:
	config->stop_request = 0;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
    	LOG(ERROR) << ERR_SOCKET_CREATE;
    	return ERRCODE_SOCKET_CREATE;
    }

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
    if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
    	LOG(ERROR) << ERR_SOCKET_CONNECT << config->intface;
    	close(sock);
    	return ERRCODE_SOCKET_CONNECT;
    }

    // open text file or read from stdin
    std::istream *strm;
    if (!config->file_name.empty())
    {
    	strm = new std::ifstream(config->file_name.c_str());
    }
    else
    {
    	strm = &std::cin;
    }

    ProtobufDeclarations pd(config->proto_path, config->verbosity);
    Pkt2OptionsCache poc(&pd);
    // read lines from the file or stdin
    while ((!config->stop_request) && (!strm->eof()))
    {
        // send message to the nano queue
    	std::string line;
    	*strm >> line;
    	void *buffer = NULL;
    	size_t sz = parseLine(&pd, &poc, line, &buffer);
    	if (sz)
    	{
			int n = write(sock, buffer, sz);
			if (n < 0)
			{
				LOG(ERROR) << ERR_SOCKET_SEND << config->intface << " " << n;
			}
			free(buffer);
    	}
    	else
    		LOG(ERROR) << ERR_PARSE_LINE << line ;
	}
    if (!config->file_name.empty())
    {
    	delete strm;
    }

   	close(sock);
	
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
