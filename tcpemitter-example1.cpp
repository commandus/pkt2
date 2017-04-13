#include <string>
#include <iostream>
#include <utilstring.h>
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
#define DEF_DELAY_SEC		1

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
        uint8_t tag;			///< always 0xff
} PACKED EXAMPLE1PACKET;

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

int main(int argc, char **argv)
{
    // Signal handler
    setSignalHandler(SIGINT);

    struct arg_str *a_intface = arg_str0("i", "intface", "<host>", "Host name or address. Default 0.0.0.0");
    struct arg_int *a_port = arg_int0("p", "port", "<port number>", "Destination port. Default 50052.");
    struct arg_int *a_delay = arg_int0("d", "delay", "<seconds>", "Delay after send packet. Default 1 (0- no delay).");
    struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 2, "Verbosity level");
    struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
    struct arg_end *a_end = arg_end(20);

    void* argtable[] = {a_intface, a_port, a_delay, a_verbosity, a_help, a_end};
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
    int verbosity;
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
		delay = DEF_DELAY_SEC;
	verbosity = a_verbosity->count;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	time_t t = time(NULL);
	srand(t);
	int count = 0;
	cont = true;
	int sock;
	EXAMPLE1PACKET pkt;

	while (cont)
	{
		for (int d = 0; cont && (d < 3); d++)
		{
			int r = open_socket(sock, intface, port);
			if (r == ERR_OK)
			{
				pkt.device = d;
				pkt.unixtime = htonl(time(NULL));
				double c = 20.0 + ((10. * rand()) / RAND_MAX);
				int16_t cc = c / 1.22;
				pkt.temperature = htons(cc);
				pkt.tag = 0xff;

				int n = write(sock, &pkt, sizeof(pkt));
				if (n < 0)
					std::cerr << ERR_SOCKET_SEND << intface << ":" << port << ". " << strerror(errno) << std::endl;
				shutdown(sock, SHUT_RDWR);
				close(sock);
				if (verbosity >= 1)
				{
					std::cerr << count;
					if (verbosity >= 2)
					{
						std::cerr << " " << count << " " << d << " " << c;
					}
					std::cerr << std::endl;
				}
			}
			else
			{
				std::cerr << count << std::endl;
			}
			if (delay > 0)
				sleep(delay);
			count++;
		}
	}
}
