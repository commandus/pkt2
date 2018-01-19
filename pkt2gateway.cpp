#include <iostream>

#include <string>
#include <iostream>
#include <signal.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <argtable2.h>

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "platform.h"
#include "pkt2gateway-config.h"

#include "errorcodes.h"
#include "utilstring.h"
#include "utilprotobuf.h"
#include "packet2message.h"

bool cont;

Config *config;

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		cont = false;
		std::cerr << MSG_INTERRUPTED;
		break;
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST << " nothing to do";
		break;
	default:
		break;
	}
}

void setSignalHandler(int signal)
{
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_handler = &signalHandler;
        sigaction(signal, &action, NULL);
}

/**
 * Read packet as string from the file, stdin ot command line
 * @param config
 * @return
 */
std::string readPacket
(
		Config *config
)
{
	std::string packet;
	if (config->packet.empty())
	{
		if (config->file_name.empty())
		{
			// read from stdin
			packet = file2string(std::cin);
		}
		else
		{
			// read from file
			packet = file2string(config->file_name);
		}
	}
	else
	{
		// read from command parameter
		packet = config->packet;
	}
	if (config->mode == MODE_HEX)
		packet = hex2string(packet);
	return packet;
}

int main(int argc, char **argv)
{
	config = new Config(argc, argv);
    // Signal handler
    setSignalHandler(SIGINT);
	setSignalHandler(SIGHUP);

	if (!config)
		exit(ERRCODE_PARSE_COMMAND);
    if (config->error() != 0)
	{
		std::cerr << ERR_COMMAND << std::endl;
		exit(ERRCODE_COMMAND);
	}
    // Read packet
    std::string packet = readPacket(config);
    if (packet.empty())
    {
		std::cerr << ERR_COMMAND << std::endl;
		exit(ERRCODE_COMMAND);
    }

    if (config->verbosity >= 2)
    {
    	std::cerr << MSG_PACKET_HEX << hexString(packet) << std::endl;
    }

	// open socket to write
	int nano_socket_out = nn_socket(AF_SP, NN_BUS);
	int eoutid = nn_bind(nano_socket_out, config->message_out_url.c_str());
	if (eoutid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->message_out_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

    ProtobufDeclarations declarations(config->proto_path, config->verbosity);
    Pkt2OptionsCache options_cache(&declarations);
    Packet2Message packet2Message(&declarations, &options_cache, config->verbosity);

    PacketParseEnvironment packet_env(NULL, NULL, packet, &options_cache, config->force_message);

    google::protobuf::Message *m = packet2Message.parsePacket(&packet_env);

    if (m == NULL)
    {
		std::cerr << ERR_PARSE_PACKET << std::endl;
		exit(ERRCODE_PARSE_PACKET);
    }

	cont = true;
	
	// send message
	for (int d = 0; d < config->retries; d++)
	{
		if (!cont)
			break;
		MessageTypeNAddress messageTypeNAddress(m->GetTypeName());
		std::string outstr = stringDelimitedMessage(&messageTypeNAddress, *m);
		int sent = nn_send(nano_socket_out, outstr.c_str(), outstr.size(), 0);
		if (sent < 0)
			LOG(ERROR) << ERR_NN_SEND << sent;
		else
		{
			if (config->verbosity >= 1)
			{
				LOG(INFO) << MSG_SENT << sent << " " << hexString(outstr);
			}
		}
		SLEEP(config->retry_delay);	// BUGBUG Pass 0 for https://github.com/nanomsg/nanomsg/issues/182
	}

	int r = nn_shutdown(nano_socket_out, eoutid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->message_out_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}

	delete m;
}
