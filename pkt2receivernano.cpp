#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

#include <glog/logging.h>

#include "pkt2receivernano.h"
#include "input-packet.h"
#include "errorcodes.h"

/**
  * @return: 0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
  */
int pkt2_receiever_nano(Config *config)
{
    int nn_sock_in = nn_socket(AF_SP, NN_SUB);
    int r = nn_setsockopt(nn_sock_in, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
	if (r < 0)
	{
			LOG(ERROR) << ERR_NN_SUBSCRIBE << config->in_url << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_SUBSCRIBE;
	}
    if (nn_connect(nn_sock_in, config->in_url.c_str()) < 0)
    {
        LOG(ERROR) << ERR_NN_CONNECT << config->in_url;
		return ERRCODE_NN_CONNECT;
    }

    while (!config->stop_request)
    {
        char *buf = NULL;
        int bytes = nn_recv(nn_sock_in, &buf, NN_MSG, 0);

        if (bytes < 0)
        {
        	LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
            continue;
        }
        if (buf)
        {
            InputPacket packet(buf, bytes);

    		LOG(ERROR) << "packet " << std::string(1, packet.header()->name) << " " <<
    				inet_ntoa(packet.get_sockaddr_src()->sin_addr) << ":" << ntohs(packet.get_sockaddr_src()->sin_port) << "->" <<
					inet_ntoa(packet.get_sockaddr_dst()->sin_addr) << ":" << ntohs(packet.get_sockaddr_dst()->sin_port) << " " << packet.length << " bytes.";

            if (packet.error() != 0)
            {
                LOG(ERROR) << ERRCODE_PACKET_PARSE << packet.error();
                continue;
            }
            nn_freemsg(buf);
        }
	}
    return nn_shutdown(nn_sock_in, 0);
}

/**
  * @return 0- success
  *         ERRCODE_STOP- config is not initialized yet
  */
int stop(Config *config)
{
    if (!config)
        return ERRCODE_STOP;
    config->stop_request = true;
    return ERR_OK;
    // wake up

}
