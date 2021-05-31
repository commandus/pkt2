#include "argtable3/argtable3.h"
#include <limits.h>
#include <stdlib.h>
#include "pkt2js-config.h"
#include "errorcodes.h"

#if defined(_WIN32) || defined(_WIN64)
#else
#include <unistd.h>
#endif

#define DEF_PROTO_PATH				"proto"
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
	struct arg_str *a_in_url = arg_str0("i", "input", "<file>", "Default stdin" );
	struct arg_str *a_out_url = arg_str0("o", "output", "<file>", "Default stdout");
	struct arg_str *a_mode = arg_str0("m", "output_mode", "<mode>", "Output mode: json(default), csv, tab, sql, Sql, pbtext, dbg, hex, bin, CSV, TAB- with headers");

	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	struct arg_str *a_force_message = arg_str0("f", "message", "<name>", "force packet.message");
	struct arg_int *a_allowed_packet_sizes = arg_intn("a", "allow", "<size>", 0, 128, "Allowed payload packet size. Default any.");

	struct arg_lit *a_input_hex = arg_lit0("x", "hex", "Input hex string, Default binary.");
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
		a_in_url, a_out_url, a_mode,
		a_proto_path, a_force_message, a_allowed_packet_sizes,
		a_input_hex, a_buffer_size,
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
		if (a_help->count)
			return ERRCODE_HELP_REQUESTED;
		else
			return ERRCODE_COMMAND;
	}

	if (a_in_url->count)
		filenameInput = *a_in_url->sval;
	else
		filenameInput = "";	// means stdin
	if (a_out_url->count)
		filenameOutput = *a_out_url->sval;
	else
		filenameOutput = "";	// means stdout
	output_mode = 0;
	if (a_mode->count) {
		char m = **a_mode->sval;
		switch (m) {
		case 'j': // json
			output_mode = 0;
			break;
		case 'c': // csv
			output_mode = 1;
			break;
		case 't': // tab
			output_mode = 2;
			break;
		case 'C': // csv with header
			output_mode = 11;
			break;
		case 'T': // tab with header
			output_mode = 12;
			break;
		case 's': // sql
			output_mode = 3;
			break;
		case 'S': // sql2
			output_mode = 4;
			break;
		case 'p': // pbtext
			output_mode = 5;
			break;
		case 'd': // dbg
			output_mode = 6;
			break;
		case 'h': // hex
			output_mode = 7;
			break;
		case 'b': // bin
			output_mode = 8;
			break;
		} 
	}
	if (a_input_hex->count) {
		input_mode = 1;
	}

	if (a_proto_path->count)
		proto_path = *a_proto_path->sval;
	else
		proto_path = DEF_PROTO_PATH;

	// get real path
	char b[PATH_MAX];
	proto_path = std::string(realpath(proto_path.c_str(), b));

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

	for (int i = 0; i < a_allowed_packet_sizes->count; i++) {
		allowed_packet_sizes.push_back(a_allowed_packet_sizes->ival[i]);
	}
	
	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	if (a_max_fd->count)
		max_fd = *a_max_fd->ival;
	else
		max_fd = 0;

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

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return ERR_OK;
}
