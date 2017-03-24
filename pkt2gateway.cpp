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
		}
		else
		{
			// read from file
		}
	}
	else
	{
		// read from command parameter
	}
	if (config->mode == MODE_HEX)

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
		std::cerr << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
	}

    // Read packet
    std::string packet = readPacket(config);
    if (packet.empty())
    {
		std::cerr << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
    }

    google::protobuf::Message *m = Packet2Message::parse(config->proto_path, packet);

    if (m == NULL)
    {
		std::cerr << ERR_PARSE_PACKET;
		exit(ERRCODE_PARSE_PACKET);
    }

	for (int d = 0; d < config->retries; d++)
	{
		if (config->retry_delay > 0)
			sleep(config->retry_delay);
	}

	delete m;
}
