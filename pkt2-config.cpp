#include "pkt2-config.h"
#include <unistd.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <argtable3.h>

#define DEF_PROTO_PATH				"proto"
#define DEF_QUEUE_OUT               "ipc:///tmp/message.pkt2"
#define DEF_CFG						"pkt2.js"
#define DEF_PATH					"."

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
	struct arg_file *a_input_file = arg_file0("c", "config", "<file name>", "Configuration file name. Default " DEF_CFG);
	struct arg_file *a_path = arg_file0("p", "path", "<path>", "Working directory path. Default " DEF_PATH);
	struct arg_lit *a_output = arg_lit0("o", "output", "Show status");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
		a_input_file, a_path, a_output, a_daemonize, a_verbosity, 
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

	// get real path
	char b[PATH_MAX];

	if (a_input_file->count)
		file_name = *a_input_file->filename;
	else
		file_name = DEF_CFG;	
	file_name = std::string(realpath(file_name.c_str(), b));

	if (a_path->count)
		path = *a_path->filename;
	else
	{
		char wd[PATH_MAX];
		path = getcwd(wd, PATH_MAX);	
	}

	// get real path
	path = std::string(realpath(path.c_str(), b));

	mode_output = a_output->count;

	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

