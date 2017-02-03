#include "write2lmdb-config.h"
#include <argtable2.h>

#define DEF_DB_PATH              "db"
#define DEF_MODE                 0664
#define DEF_FLAGS                0
#define DEF_QUEUE                "ipc:///tmp/output.pkt2"

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
        struct arg_str *a_message_url = arg_str0("q", "queue", "<queue url>", "Default ipc:///tmp/input.pkt2");
        struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
        struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
        struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
        struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 4, "Verbosity level");

        struct arg_str *a_db_path = arg_str0("p", "dbpath", "<path>", "Default db");
        struct arg_int *a_flags = arg_int0("f", "flags", "<number>", "LMDB flags. Default 0");
        struct arg_int *a_mode = arg_int0("m", "mode", "<number>", "LMDB file open mode. Default 0664");

        struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
        struct arg_end *a_end = arg_end(20);

        void* argtable[] = { 
                a_message_url,
                a_retries, a_retry_delay,
                a_daemonize, a_verbosity,
				a_db_path, a_flags, a_mode,
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

        if (a_message_url->count)
                message_url = *a_message_url->sval;
        else
                message_url = DEF_QUEUE;

        if (a_retries->count)
                retries = *a_retries->ival;
        else
                retries = 0;

        if (a_retry_delay->count)
                retry_delay = *a_retry_delay->ival;
        else
                retry_delay = 60;
        
        daemonize = a_daemonize->count > 0;

        if (a_db_path->count)
        	path = *a_db_path->sval;
        else
            path = DEF_DB_PATH;

        if (a_mode->count)
            mode = *a_mode->ival;
        else
            mode = DEF_MODE;

        if (a_flags->count)
        	flags = *a_flags->ival;
        else
        	flags = DEF_FLAGS;

        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
}