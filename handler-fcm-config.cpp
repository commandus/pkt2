#include "handler-fcm-config.h"
#include <limits.h>
#include <stdlib.h>
#include "argtable3/argtable3.h"
#include <strings.h>
#include <libpq-fe.h>

#if defined(_WIN32) || defined(_WIN64)
#else
#include <unistd.h>
#endif

#include "errorcodes.h"

#define DEF_DB_PATH              	"db"
#define DEF_MODE                 	0664
#define DEF_FLAGS              		0
#define DEF_QUEUE                	"ipc:///tmp/message.pkt2"
#define DEF_PROTO_PATH				"proto"

// PostgreSQL
#define DEF_DB_HOST             	"localhost"
#define DEF_DB_PORT             	"5432"
#define DEF_DATABASESOCKET          ""
#define DEF_DATABASECHARSET         "utf8"
#define DEF_DATABASECLIENTFLAGS     0

#define DEF_BUFFER_SIZE				4096
#define	DEF_SERVER_KEY				"AAAAITL4VBA:APA91bGQwuvaQTt8klgebh8QO1eSU7o5itF0QGnp7kCWJNgMwe8WM3bMh6eGDkeyMbvUAmE2MqtB1My3f0-mHM6MQE1gOjMB0eiAW1Xaqds0hYETRNzqAe0iRh5v-PcxmxrHQeJh6Nuj";
#define DEF_IMEI_FIELD				"imei"

Config::Config
(
    int argc,
    char* argv[]
)
{
	stop_request = 0;
	lastError = parseCmd(argc, argv);
	fburl = FCM_SEND;
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

	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	
	// FireBase server key
	struct arg_str *a_server_key = arg_str0("k", "key", "<key>", "FireBase server token");
	struct arg_str *a_imei_field_name = arg_str0("m", "imei", "<IMEI>", "Field name contains IMEI. Default " DEF_IMEI_FIELD);
	
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

	struct arg_int *a_format_number = arg_int0(NULL, "format", "<number>", "Default 0");

	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Receiver buffer size. Default 4096");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
			a_proto_path,
			a_message_url, a_allowed_messages,
			a_retries, a_retry_delay,
			a_daemonize, a_verbosity,
			a_server_key, a_imei_field_name,
			a_conninfo, a_user, a_database, a_password, a_host, a_dbport, a_optionsfile, a_dbsocket, a_dbcharset, a_dbclientflags,
			a_format_number, a_buffer_size,
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
	proto_path = std::string(realpath(proto_path.c_str(), b));

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

	if (a_server_key->count)
		server_key = *a_server_key->sval;
	else
		server_key = DEF_SERVER_KEY;
	
	if (a_imei_field_name->count)
		imei_field_name = *a_imei_field_name->sval;
	else
		imei_field_name = DEF_IMEI_FIELD;

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

	if (a_format_number->count)
		format_number = *a_format_number->ival;
	else
		format_number = 0;

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
