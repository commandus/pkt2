#include "tcpemitter-config.h"
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include "argtable3/argtable3.h"

#define DEF_PROTO_PATH				"proto"
#define DEF_QUEUE_OUT               "ipc:///tmp/message.pkt2"

Config::Config
(
    int argc,
    char* argv[]
)
{
    stop_request = 0;
	lastError = parseCmd(argc, argv);
}

int Config::error()
{
    return lastError;
}

/**
 * @brief Parse command line into struct ClientConfig
 * @return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int Config::parseCmd
(
        int argc,
        char* argv[]
)
{
    struct arg_str *a_interface = arg_str0("i", "ipaddr", "<IP address>", "Network interface name or address. Default 0.0.0.0");
    struct arg_int *a_port = arg_int0("l", "listen", "<port>", "TCP port to listen. Default 50052");

	struct arg_str *a_message_out_url = arg_str0("o", "output", "<queue url>", "Default " DEF_QUEUE_OUT);
	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	struct arg_file *a_input_file = arg_file0("f", "file", "<file name>", "otherwise -i from nanomsg socket or read stdin");
	struct arg_int *a_count = arg_int0("n", "count", "<n>", "Repeat each message. Default 1.");
	struct arg_int *a_delay = arg_int0("w", "delay", "<seconds>", "Delay on repeats in seconds. Default 0");
	struct arg_int *a_retries = arg_int0("r", "restart", "<n>", "Start after error. Default 1.");
	struct arg_int *a_retry_delay = arg_int0("y", "restartdelay", "<seconds>", "Delay on restart, seconds. Default 0");

	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
			a_interface, a_port, a_proto_path,
			a_input_file, a_message_out_url, a_daemonize,
			a_count, a_delay, a_retries, a_retry_delay, a_verbosity,
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

	if (a_proto_path->count)
		proto_path = *a_proto_path->sval;
	else
		proto_path = DEF_PROTO_PATH;

	// get real path
	char b[PATH_MAX];
	proto_path = std::string(realpath(proto_path.c_str(), b));

	if (a_input_file->count)
		file_name = *a_input_file->filename;
	else
		file_name = "";	// stdin

	if (a_message_out_url->count)
		message_out_url = *a_message_out_url->sval;
	else
		message_out_url = DEF_QUEUE_OUT;

	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	if (a_retries->count)
		retries = *a_retries->ival;
	else
		retries = 0;

	if (a_count->count)
		count = *a_count->ival;
	else
		count = 1;

	if (a_delay->count)
		delay = *a_delay->ival;
	else
		delay = 0;

	if (a_retry_delay->count)
			retry_delay = *a_retry_delay->ival;
	else
			retry_delay = 60;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

