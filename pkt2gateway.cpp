#include <iostream>
#include "pkt2gateway-config.h"

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

#include <google/protobuf/message.h>

#include "errorcodes.h"
#include "utilstring.h"
#include "packet2message.h"

bool cont;

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		cont = false;
		std::cerr << "Interrupted" << std::endl;
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
    // Signal handler
    setSignalHandler(SIGINT);


    Config *config = new Config(argc, argv);
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

    Packet2Message packet2Message(config->proto_path, config->verbosity);
    google::protobuf::Message *m = packet2Message.parse(packet, config->force_message);

    if (m == NULL)
    {
		std::cerr << ERR_PARSE_PACKET << std::endl;
		exit(ERRCODE_PARSE_PACKET);
    }

	for (int d = 0; d < config->retries; d++)
	{
		if (config->retry_delay > 0)
			sleep(config->retry_delay);
	}

	delete m;
}
