#include "tcpreceiver-config.h"
#include <argtable2.h>

#define PROGRAM_NAME             "tcpreceiver"
#define PROGRAM_DESCRIPTION      "PKT2 tcp packet listener"
#define DEF_PORT                 50055
#define DEF_ADDRESS              "0.0.0.0"

Config::Config
(
    int argc, 
    char* argv[]
)
{
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
        struct arg_int *a_port = arg_int0("l", "listen", "<port>", "service port. Default 50055");

        struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
        struct arg_end *a_end = arg_end(20);

        void* argtable[] = { a_interface, a_port,
                a_help, a_end };

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

        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
}
