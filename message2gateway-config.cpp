#include "message2gateway-config.h"
#include <limits.h>
#include <stdlib.h>
#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <direct.h>
#include "platform.h"
#include "utilfile.h"
#else
#include <unistd.h>
#endif
#include "argtable3/argtable3.h"

#include "errorcodes.h"

#define DEF_PROTO_PATH				"proto"
#define DEF_QUEUE_IN                "ipc:///tmp/packet.pkt2"
#define DEF_QUEUE_OUT               "ipc:///tmp/message.pkt2"
#define DEF_BUFFER_SIZE				4096

Config::Config
(
	int argc,
	char* argv[]
)
{
	stop_request = 0;

	lastError = parseCmd(argc, argv);
	buffer_size = DEF_BUFFER_SIZE;
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
	struct arg_str *a_message_in_url = arg_str0("i", "input", "<queue url>", "nanomsg socket queue " DEF_QUEUE_IN);
	struct arg_str *a_message_out_url = arg_str0("o", "output", "<queue url>", "Default " DEF_QUEUE_OUT);
	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	struct arg_file *a_input_file = arg_file0("f", "file", "<file name>", "otherwise -i from nanomsg socket or -s read stdin");
	struct arg_lit *a_stdin = arg_lit0("s", "stdin", "read stdin");

	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<bytes>", "with -i option only. Socket buffer size. Default 4096.");

	struct arg_str *a_force_message = arg_str0(NULL, "force", "<packet.message>", "force packet.message");

	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");

	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
			a_proto_path,
			a_message_in_url, a_input_file, a_stdin, a_message_out_url,
			a_force_message,
			a_buffer_size, a_retries, a_retry_delay, a_daemonize,
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

	if (a_proto_path->count)
		proto_path = *a_proto_path->sval;
	else
		proto_path = DEF_PROTO_PATH;

	// get real path
	char b[PATH_MAX];
	char *pp = realpath(proto_path.c_str(), b);
	if (pp)
		proto_path = std::string(pp);
	else {
		std::cerr << ERR_INVALID_PROTO_PATH << std::endl;
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return ERRCODE_INVALID_PROTO_PATH;
	}

	if (a_input_file->count)
		file_name = *a_input_file->filename;
	else
		file_name = "";	// stdin

	from_stdin = (a_stdin->count > 0);

	if (a_message_in_url->count)
			message_in_url = *a_message_in_url->sval;
	else
			message_in_url = DEF_QUEUE_IN;

	if (a_message_out_url->count)
			message_out_url = *a_message_out_url->sval;
	else
			message_out_url = DEF_QUEUE_OUT;

	if (a_retries->count)
			retries = *a_retries->ival;
	else
			retries = 0;

	if (a_retry_delay->count)
			retry_delay = *a_retry_delay->ival;
	else
			retry_delay = 60;

	if (a_force_message->count)
		force_message = *a_force_message->sval;
	else
		force_message = "";

	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	if (a_buffer_size->count)
		buffer_size = *a_buffer_size->ival;
	else
		buffer_size = DEF_BUFFER_SIZE;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}
