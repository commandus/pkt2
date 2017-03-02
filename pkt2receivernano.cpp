#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

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
    int nano_socket = nn_socket(AF_SP, NN_PUSH);
    if (nn_connect(nano_socket, config->message_url.c_str()) < 0)
    {
        LOG(ERROR) << ERR_NN_CONNECT << config->message_url;
		return ERRCODE_NN_CONNECT;
    }

    while (!config->stop_request)
    {
        char *buf = NULL;
        int bytes = nn_recv(nano_socket, &buf, NN_MSG, 0);

        if (bytes < 0)
        {
        	LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
            continue;
        }
        if (buf)
        {
            InputPacket packet(buf, bytes);
            LOG(INFO) << "packet " << std::string(1, packet.header()->name) << " "
                << packet.get_socket_addr_src() << " ";

            if (packet.error() != 0)
            {
                LOG(ERROR) << ERRCODE_PACKET_PARSE << packet.error();
                continue;
            }
            nn_freemsg(buf);
        }
	}
    return nn_shutdown(nano_socket, 0);
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
