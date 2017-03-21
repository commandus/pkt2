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

#include "errorcodes.h"

#define PROGRAM_NAME		"tcpemitter-example1"
#define PROGRAM_DESCRIPTION "Send packets see proto/example/example1.proto"

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

#ifdef _MSC_VER
#define ALIGN   __declspec(align(1))
#define PACKED
#else
#define ALIGN
#define PACKED  __attribute__((aligned(1), packed))
#endif

typedef ALIGN struct EXAMPLE1PACKET {
        uint8_t device;
        uint32_t unixtime;
        int16_t temperature;
} PACKED EXAMPLE1PACKET;

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

    struct hostent *server = gethostbyname(host.c_str());

    if (server == NULL)
    {
    	std::cerr << ERR_GET_ADDRINFO << host << std::endl;;
    	return ERRCODE_GET_ADDRINFO;
    }

    struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	// int optval = 1;
	// setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
    	std::cerr << ERR_SOCKET_CONNECT << host << ":" << port << ". " << strerror(errno) << std::endl;
    	close(sock);
    	return ERRCODE_SOCKET_CONNECT;
    }
    return ERR_OK;
}

int main(int argc, char **argv)
{
    // Signal handler
    setSignalHandler(SIGINT);

    struct arg_str *a_intface = arg_str0("i", "intface", "<host>", "Host name or address. Default 0.0.0.0");
    struct arg_int *a_port = arg_int0("p", "port", "<port number>", "Destination port. Default 50052.");
    struct arg_int *a_delay = arg_int0("d", "delay", "<seconds>", "Delay after send packet. Default 0 (no delay).");
    struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
    struct arg_end *a_end = arg_end(20);

    void* argtable[] = {a_intface, a_port, a_delay, a_help, a_end};
	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		exit(ERRCODE_COMMAND);
	}
	// Parse the command line as defined by argtable[]
	int nerrors = arg_parse(argc, argv, argtable);
	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)
	{
			if (nerrors)
					arg_print_errors(stderr, a_end, PROGRAM_NAME);
			printf("Usage: %s\n", PROGRAM_NAME);
			arg_print_syntax(stdout, argtable, "\n");
			printf("%s\n", PROGRAM_DESCRIPTION);
			arg_print_glossary(stdout, argtable, "  %-25s %s\n");
			arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
			exit(ERRCODE_PARSE_COMMAND);
	}

	std::string intface;
    int port;
    int delay;
	if (a_intface->count)
		intface = *a_intface->sval;
	else
		intface = "0.0.0.0";
	if (a_port->count)
		port = *a_port->ival;
	else
		port = 50052;
	if (a_delay->count)
		delay = *a_delay->ival;
	else
		delay = 0;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	time_t t = time(NULL);
	srand(t);
	int count = 0;
	cont = true;

	int sock;
	EXAMPLE1PACKET pkt;

	while (cont)
	{
		for (int d = 0; d < 3; d++)
		{
			int r = open_socket(sock, intface, port);
			if (r == ERR_OK)
			{
				pkt.device = d;
				pkt.unixtime = htonl(time(NULL));
				double c = 20.0 + ((10. * rand()) / RAND_MAX);
				int16_t cc = c / 1.22;
				pkt.temperature = htons(cc);

				int n = write(sock, &pkt, sizeof(pkt));
				if (n < 0)
				{
					std::cerr << ERR_SOCKET_SEND << intface << ":" << port << ". " << strerror(errno) << std::endl;
					close(sock);
					r = !ERR_OK;
				}
				close(sock);
			}
			else
			{
			}
			if (delay > 0)
				sleep(delay);
			count++;
		}
	}
}
