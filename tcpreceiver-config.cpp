#include "tcpreceiver-config.h"
#include <argtable2.h>

#define DEF_PORT                 50055
#define DEF_ADDRESS              "0.0.0.0"
#define DEF_QUEUE                "ipc:///tmp/input.pkt2"
#define DEF_BUFFER_SIZE          256

Config::Config
(
    int argc, 
    char* argv[]
)
{
        stop_request = false;
        lastError = parseCmd(argc, argv);
}

int Config::error() 
{
    return lastError;
}

/**
 * Parse command line into struct ClientConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int Config::parseCmd
(
        int argc,
        char* argv[]
)
{
        struct arg_str *a_interface = arg_str0("i", "ipaddr", "<IP address>", "Netrwotk interface name or address. Default 0.0.0.0");
        struct arg_int *a_port = arg_int0("l", "listen", "<port>", "TCP port to listen. Default 50055");

        struct arg_str *a_message_url = arg_str0("q", "queue", "<queue url>", "Default ipc:///tmp/input.pkt2");
        struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Default 256 bytes");
        struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
        struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 4, "Verbosity level");

        struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
        struct arg_end *a_end = arg_end(20);

        void* argtable[] = { 
                a_interface, a_port, a_message_url, a_buffer_size, a_daemonize,
                a_verbosity,
                a_help, a_end 
        };

        int nerrors;

        // verify the argtable[] entries were allocated successfully
        if (arg_nullcheck(argtable) != 0)
        {
            arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
            return 1;
        }
        // Parse the command line as defined by argtable[]
        nerrors = arg_parse(argc, argv, argtable);

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
                return 1;
        }

        if (a_interface->count)
                intface = *a_interface->sval;
        else
                intface = DEF_ADDRESS;

        if (a_port->count)
                port = *a_port->ival;
        else
                port = DEF_PORT;

        if (a_message_url->count)
                message_url = *a_message_url->sval;
        else
                message_url = DEF_QUEUE;

        if (a_buffer_size->count)
                buffer_size = *a_buffer_size->ival;
        else
                buffer_size = DEF_BUFFER_SIZE;

        daemonize = a_daemonize->count > 0;

        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
}
