#include "freceiver-config.h"
#include <unistd.h>
#include <limits.h>
#include <cstring>
#include <stdlib.h>
#include <argtable3.h>

#define DEF_QUEUE                "ipc:///tmp/packet.pkt2"
#define DEF_BUFFER_SIZE          4096

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
	// input options
	struct arg_str *a_cmd_text = arg_str0("t", "text", "<hex|int>", "1, hex: hex lines; 2, int: lines or decimal per line. Default 0: binary");
	struct arg_file *a_filename_in = arg_file0("i", "input", "<file>", "Default read stdin.");
	struct arg_int *a_packet_size = arg_int0("s", "size", "<bytes>", "Packet size. Default 0- to EOL or EOF");
	struct arg_str *a_message_url = arg_str0("o", "output", "<bus url>", "Default ipc:///tmp/packet.pkt2");
	
	struct arg_int *a_print_mode = arg_int0("m", "print", "<mode>", "Just print to stdout parsed packets. 0- JSON, CSV- 1, Tab delimited- 2, SQL- 3, SQL(2)- 4, Protobuf text- 5, Debug- 6");
	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	
	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Default 4096 bytes");
	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_int *a_max_fd = arg_int0(NULL, "maxfd", "<number>", "Set max file descriptors. 0- use default (1024).");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
			a_cmd_text, a_filename_in, a_packet_size, a_message_url, 
			a_print_mode, a_proto_path,
			a_buffer_size, a_retries, a_retry_delay,
			a_daemonize, a_max_fd, a_verbosity,
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

	file_mode = FILE_MODE_BIN;
	if (a_cmd_text->count)
	{
		file_mode = FILE_MODE_BIN;
		if (strcmp(*a_cmd_text->sval, "hex") == 0)
			file_mode = FILE_MODE_TEXT_HEX;
		if (strcmp(*a_cmd_text->sval, "int") == 0)
			file_mode = FILE_MODE_TEXT_INT;
		if (strcmp(*a_cmd_text->sval, "1") == 0)
			file_mode = FILE_MODE_TEXT_HEX;
		if (strcmp(*a_cmd_text->sval, "2") == 0)
			file_mode = FILE_MODE_TEXT_INT;
	}

	if (a_filename_in->count)
		filename_in = std::string(*a_filename_in->filename);
	else
		filename_in = "";

	if (a_packet_size->count)
		packet_size = *a_packet_size->ival;
	else
		packet_size = 0;

	if (a_message_url->count)
		message_url = *a_message_url->sval;
	else
		message_url = DEF_QUEUE;

	if (a_proto_path->count)
		proto_path = *a_proto_path->sval;
	else
		proto_path = DEF_PROTO_PATH;

	// get real path
	char b[PATH_MAX];
	proto_path = std::string(realpath(proto_path.c_str(), b));

	if (a_print_mode->count)
		mode = *a_print_mode->ival;
	else
		mode = -1;

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

	if (a_max_fd > 0)
		max_fd = *a_max_fd->ival;
	else
		max_fd = 0;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}
