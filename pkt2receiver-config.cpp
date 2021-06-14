#include "argtable3/argtable3.h"
#include <stdlib.h>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#else
#include <limits.h>
#include <unistd.h>
#endif

#include "pkt2receiver-config.h"
#include "errorcodes.h"

#define DEF_PROTO_PATH				"proto"
#define DEF_IN_QUEUE				"ipc:///tmp/packet.pkt2"
#define DEF_OUT_QUEUE				"ipc:///tmp/message.pkt2"
#define DEF_CONTROL_QUEUE			"ipc:///tmp/control.pkt2"
#define DEF_DUMP_QUEUE				"ipc:///tmp/dump.pkt2"
#define DEF_BUFFER_SIZE 	         4096

/**
  * @brief Parse command line
  * @see pktreceiver.cpp
  */
Config::Config
(
	int argc, 
	char* argv[]
)
{
	stop_request = 0;
	lastError = parseCmd(argc, argv);
}

/**
  * @return 
  * 	0- success
  * 	1- show help and exit, or command syntax error
  * 	2- output file does not exists or can not open to write
  */
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
	struct arg_str *a_in_url = arg_str0("i", "input", "<queue url>", "Default " DEF_IN_QUEUE);
	struct arg_str *a_out_url = arg_str0("o", "output", "<queue url>", "Default " DEF_OUT_QUEUE);
	struct arg_str *a_control_url = arg_str0(NULL, "control", "<queue url>", "Default " DEF_CONTROL_QUEUE);
	struct arg_str *a_dump_url = arg_str0(NULL, "dump", "<dump url>", "Default " DEF_DUMP_QUEUE);

	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	struct arg_str *a_force_message = arg_str0(NULL, "message", "<packet.message>", "force message");
	struct arg_int *a_allowed_packet_sizes = arg_intn("a", "allow", "<size>", 0, 128, "Allowed payload packet size. Default any.");

	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Default 4096 bytes");
	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_int *a_max_fd = arg_int0(NULL, "maxfd", "<number>", "Set max file descriptors. 0- use default (1024).");

   	struct arg_str *a_table_alias = arg_strn("T", "table-alias", "<alias=message>", 0, 100, "set table alias for message");
    struct arg_str *a_field_alias = arg_strn("F", "field-alias", "<alias=field>", 0, 100, "set field alias");

	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
		a_in_url, a_out_url, a_control_url, a_dump_url,
		a_proto_path, a_force_message, a_allowed_packet_sizes,
		a_buffer_size,
		a_retries, a_retry_delay, a_max_fd, 
		a_table_alias, a_field_alias,
		a_daemonize, a_verbosity,
		a_help, a_end 
	};

	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return ERRCODE_PARSE_COMMAND;
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
		return ERRCODE_COMMAND;
	}


	if (a_in_url->count)
		in_url = *a_in_url->sval;
	else
		in_url = DEF_IN_QUEUE;
	if (a_out_url->count)
		out_url = *a_out_url->sval;
	else
		out_url = DEF_OUT_QUEUE;
	if (a_control_url->count)
		control_url = *a_control_url->sval;
	else
		control_url = DEF_CONTROL_QUEUE;
	if (a_dump_url->count)
		dump_url = *a_dump_url->sval;
	else
		dump_url = DEF_DUMP_QUEUE;

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

	if (a_force_message->count)
		force_message = *a_force_message->sval;
	else
		force_message = "";

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

	for (int i = 0; i < a_allowed_packet_sizes->count; i++)
	{
		allowed_packet_sizes.push_back(a_allowed_packet_sizes->ival[i]);
	}
	
	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	if (a_max_fd->count)
		max_fd = *a_max_fd->ival;
	else
		max_fd = 0;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	for (int i = 0; i < a_table_alias->count; i++) {
		std::string line = a_table_alias->sval[i];
		size_t p = line.find('=');
		if (p == std::string::npos)
			continue;
		std::string n = line.substr(0, p);
		std::string v = line.substr(p + 1);
		tableAliases[n] = v;
	}

	for (int i = 0; i < a_field_alias->count; i++) {
		std::string line = a_field_alias->sval[i];
		size_t p = line.find('=');
		if (p == std::string::npos)
			continue;
		std::string n = line.substr(0, p);
		std::string v = line.substr(p + 1);
		fieldAliases[n] = v;
	}
	
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return ERR_OK;
}
