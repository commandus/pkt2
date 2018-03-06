#include "pkt2dumppq-config.h"

#include <argtable2.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "errorcodes.h"

#define DEF_DB_PATH					"db"
#define DEF_MODE					0664
#define DEF_FLAGS					0
#define DEF_QUEUE					"ipc:///tmp/dump.pkt2"

// PostgreSQL
#define DEF_DB_HOST					"localhost"
#define DEF_DB_PORT					"5432"
#define DEF_DATABASESOCKET			""
#define DEF_DATABASECHARSET			"utf8"
#define DEF_DATABASECLIENTFLAGS		0

#define DEF_BUFFER_SIZE				4096
#define DEF_SQL_MODE				4

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
        struct arg_str *a_packet_url = arg_str0("i", "input", "<queue url>", "Default " DEF_QUEUE);

        struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
        struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
        struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
        struct arg_int *a_max_fd = arg_int0(NULL, "maxfd", "<number>", "Set max file descriptors. 0- use default (1024).");
        struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

        // database connection
		struct arg_str *a_conninfo = arg_str0(NULL, "conninfo", "<string>", "database connection");
		struct arg_str *a_user = arg_str0(NULL, "user", "<login>", "database login");
		struct arg_str *a_database = arg_str0(NULL, "database", "<scheme>", "database scheme");
		struct arg_str *a_password = arg_str0(NULL, "password", "<password>", "database user password");
		struct arg_str *a_host = arg_str0(NULL, "host", "<host>", "database host. Default localhost");
		struct arg_str *a_dbport = arg_str0(NULL, "port", "<integer>", "database port. Default 5432");
		struct arg_file *a_optionsfile = arg_file0(NULL, "options-file", "<file>", "database options file");
		struct arg_str *a_dbsocket = arg_str0(NULL, "dbsocket", "<socket>", "database socket. Default none.");
		struct arg_str *a_dbcharset = arg_str0(NULL, "dbcharset", "<charset>", "database client charset. Default utf8.");
		struct arg_int *a_dbclientflags = arg_int0(NULL, "dbclientflags", "<number>", "database client flags. Default 0.");

		struct arg_lit *a_createtable = arg_lit0(NULL, "createtable", "create a new empty table \"packet\" in database and exit.");
		
		struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Receiver buffer size. Default 4096. 0- dymanic.");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
			a_packet_url, 
			a_retries, a_retry_delay,
			a_daemonize, a_max_fd, a_verbosity,
			a_conninfo, a_user, a_database, a_password, a_host, a_dbport, a_optionsfile, a_dbsocket, a_dbcharset, a_dbclientflags,
			a_buffer_size,
			a_createtable,
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

	if (a_packet_url->count)
		packet_url = *a_packet_url->sval;
	else
		packet_url = DEF_QUEUE;

	if (a_retries->count)
		retries = *a_retries->ival;
	else
		retries = 0;

	if (a_retry_delay->count)
		retry_delay = *a_retry_delay->ival;
	else
		retry_delay = 60;

	verbosity = a_verbosity->count;
	create_table = a_createtable->count > 0;

	daemonize = a_daemonize->count > 0;
	if (a_max_fd > 0)
		max_fd = *a_max_fd->ival;
	else
		max_fd = 0;

	dbconn = *a_conninfo->sval;
	if (a_host->count)
		dbhost = *a_host->sval;
	else
		dbhost = DEF_DB_HOST;

	if (a_dbport->count)
		dbport = *a_dbport->sval;
	else
		dbport = DEF_DB_PORT;

	dboptionsfile = *a_optionsfile->filename;
	dbname = *a_database->sval;
	dbuser = *a_user->sval;
	dbpassword = *a_password->sval;
	if (a_dbsocket->count)
		dbsocket = *a_dbsocket->sval;
	else
		dbsocket = DEF_DATABASESOCKET;

	if (a_dbcharset->count)
		dbcharset = *a_dbcharset->sval;
	else
		dbcharset = DEF_DATABASECHARSET;

	if (a_dbclientflags->count)
		dbclientflags = *a_dbclientflags->ival;
	else
		dbclientflags = DEF_DATABASECLIENTFLAGS;

	if (a_buffer_size->count)
		buffer_size = *a_buffer_size->ival;
	else
		buffer_size = DEF_BUFFER_SIZE;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return ERR_OK;
}

/**
 * Establish configured database connection
 */
PGconn *dbconnect(Config *config)
{
	if (!config->dbconn.empty())
		return PQconnectdb(config->dbconn.c_str());
	else
		return PQsetdbLogin(config->dbhost.c_str(), config->dbport.c_str(), config->dboptionsfile.c_str(),
			NULL, config->dbname.c_str(), config->dbuser.c_str(), config->dbpassword.c_str());
}
