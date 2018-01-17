#include "repeator-config.h"
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include <argtable2.h>

#include "utilstring.h"
#include "errorcodes.h"

#define MAX_OUTS				100
#define DEF_BUFFER_SIZE			4096
#define DEF_IN                	"ipc:///tmp/control.pkt2"
#define DEF_OUT                	"tcp://0.0.0.0:50000"

Config::Config
(
    int argc, 
    char* argv[]
)
{
	buffer_size = DEF_BUFFER_SIZE;
	
	lastError = parseCmd(argc, argv);

	if (lastError != 0)
	{
		lastError = ERRCODE_COMMAND;
		return;
	}
}

Config::~Config()
{
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
	struct arg_str *a_in_url = arg_str0("i", "in", "<url>", "Default " DEF_IN);
	struct arg_str *a_out_url = arg_strn("o", "out", "<url>", 0, MAX_OUTS, "One or more output. Default " DEF_OUT);
	struct arg_lit *a_bind = arg_lit0("s", "bind", "Bind socket. Default: connect to the socket.");
	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Default 4096 bytes");
	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
		a_in_url, a_out_url, a_bind,
		a_buffer_size,
		a_retries, a_retry_delay,
		a_daemonize, a_verbosity, a_help, a_end 
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
			return ERRCODE_PARSE_COMMAND;
	}

	if (a_in_url->count)
		in_url = *a_in_url->sval;
	else
		in_url = DEF_IN;

	if (a_out_url->count <= 0)
		out_urls.push_back(DEF_OUT);
	else
	{
		for (int i = 0; i < a_out_url->count; i++)
		{
			out_urls.push_back(a_out_url->sval[i]);
		}
	}
	
	bind = a_bind->count > 0;
		 
	if (a_buffer_size->count)
		buffer_size = *a_buffer_size->ival;
	else
		buffer_size = DEF_BUFFER_SIZE;

	if (a_retries->count)
		retries = *a_retries->ival;
	else
		retries = 0;

	if (a_retry_delay->count)
		retry_delay = *a_retry_delay->ival;
	else
		retry_delay = 60;

	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return ERR_OK;
}
