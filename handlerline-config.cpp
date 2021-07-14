#include "handlerline-config.h"
#include <limits.h>
#include <stdlib.h>
#include <iostream>

#include "argtable3/argtable3.h"

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <direct.h>
#include "platform.h"
#include "utilfile.h"
#else
#include <unistd.h>
#endif

#include "errorcodes.h"

#define DEF_DB_PATH              	"db"
#define DEF_MODE                 	0
#define DEF_BUFFER_SIZE				4096
#define DEF_FLAGS              		0
#define DEF_QUEUE                	"ipc:///tmp/message.pkt2"
#define DEF_PROTO_PATH				"proto"

Config::Config
(
    int argc, 
    char* argv[]
)
	: stream(NULL)
{
	stop_request = 0; ///< 0- process, 1- stop request, 2- reload request
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
	struct arg_str *a_message_url = arg_str0("i", "input", "<queue url>", "Default ipc:///tmp/message.pkt2");
	struct arg_str *a_allowed_messages = arg_strn("a", "allow", "<packet.message>", 0, 128, "Allowed message packet.name. Default any.");
	struct arg_file *a_file_name = arg_file0("o", "output", "<file name>", "write to file. Otherwise stdout");
	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	struct arg_int *a_mode = arg_int0("m", "mode", "<number>", "0- JSON, 1-CSV, 2- tab, 3- SQL, 4- SQL(2). Default 0");
	struct arg_int *a_format_number = arg_int0(NULL, "format", "<number>", "Default 0");

	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Receiver buffer size. Default 4096");

	struct arg_int *a_sql_dialect = arg_int0(NULL, "sql-dialect", "<number>", "POSTGRESQL = 0, MYSQL = 1, FIREBIRD = 2, SQLITE = 3");

   	struct arg_str *a_table_alias = arg_strn("T", "table-alias", "<alias=message>", 0, 100, "set table alias for message");
    struct arg_str *a_field_alias = arg_strn("F", "field-alias", "<alias=field>", 0, 100, "set field alias");
	struct arg_str *a_properties = arg_strn("P", "properties", "<property=name>", 0, 100, "set property");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
		a_proto_path, a_file_name, a_message_url, a_allowed_messages,
		a_retries, a_retry_delay,
		a_daemonize, a_verbosity,
		a_mode, a_format_number, a_buffer_size, a_sql_dialect,
		a_table_alias, a_field_alias, a_properties,
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
		return ERRCODE_PARSE_COMMAND;
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

	
	if (a_file_name->count)
		file_name = *a_file_name->filename;
	else
		file_name = "";

	if (a_message_url->count)
		message_url = *a_message_url->sval;
	else
		message_url = DEF_QUEUE;

	for (int i = 0; i < a_allowed_messages->count; i++)
	{
		allowed_messages.push_back(a_allowed_messages->sval[i]);
	}

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

	if (a_mode->count)
		mode = *a_mode->ival;
	else
		mode = DEF_MODE;

	if (a_format_number->count)
		format_number = *a_format_number->ival;
	else
		format_number = 0;

	if (a_buffer_size->count)
		buffer_size = *a_buffer_size->ival;
	else
		buffer_size = DEF_BUFFER_SIZE;

	sql_dialect = *a_sql_dialect->ival;

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

	for (int i = 0; i < a_properties->count; i++) {
		std::string line = a_properties->sval[i];
		size_t p = line.find('=');
		if (p == std::string::npos)
			continue;
		std::string n = line.substr(0, p);
		std::string v = line.substr(p + 1);
		properties[n] = v;
	}


	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return ERR_OK;
}
