#include "pkt2dumpfcm-config.h"

#include <argtable3.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "errorcodes.h"

#define DEF_TIME_ZONE_S				"9"
#define DEF_TIME_ZONE				9 * 3600
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

#define FCM_SEND					"https://fcm.googleapis.com/fcm/send"
#define	DEF_SERVER_KEY				"AAAAITL4VBA:APA91bGQwuvaQTt8klgebh8QO1eSU7o5itF0QGnp7kCWJNgMwe8WM3bMh6eGDkeyMbvUAmE2MqtB1My3f0-mHM6MQE1gOjMB0eiAW1Xaqds0hYETRNzqAe0iRh5v-PcxmxrHQeJh6Nuj"

#define DEF_IMEI_FIELD_OFFSET		10
#define DEF_IMEI_FIELD_OFFSET_S		"10"
#define DEF_IMEI_FIELD_SIZE			15
#define DEF_IMEI_FIELD_SIZE_S		"15"
#define DEF_PACKET_SIZE				0
#define DEF_PACKET_SIZE_S			"0- any"

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

		// FireBase server key
		struct arg_str *a_server_key = arg_str0("k", "key", "<key>", "FireBase server token");
		struct arg_int *a_imei_field_offset = arg_int0(NULL, "imei-offset", "<IMEI>", "IMEI offset. Default " DEF_IMEI_FIELD_OFFSET_S);
		struct arg_int *a_imei_field_size = arg_int0(NULL, "imei-size", "<number>", "IMEI size. Default " DEF_IMEI_FIELD_SIZE_S);
		struct arg_int *a_packet_size = arg_int0(NULL, "packet-size", "<number>", "IMEI size. Default " DEF_PACKET_SIZE_S);
		struct arg_int *a_time_zone = arg_int0(NULL, "timezone", "<number>", "Time zone offset in hours or minutes or seconds. Default " DEF_TIME_ZONE_S);
	
		// for testing purposes only
		struct arg_str *a_test_data_hex = arg_str0("x", "hex", "<packet>", "Send notification of packet and exit");


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
		
		struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Receiver buffer size. Default 4096. 0- dynamic.");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
			a_packet_url, 
			a_retries, a_retry_delay,
			a_daemonize, a_max_fd, a_verbosity,
			a_server_key, a_imei_field_offset, a_imei_field_size, a_packet_size, a_time_zone,
			a_test_data_hex,
			a_conninfo, a_user, a_database, a_password, a_host, a_dbport, a_optionsfile, a_dbsocket, a_dbcharset, a_dbclientflags,
			a_buffer_size,
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

	daemonize = a_daemonize->count > 0;
	if (a_max_fd > 0)
		max_fd = *a_max_fd->ival;
	else
		max_fd = 0;

	if (a_server_key->count)
		server_key = *a_server_key->sval;
	else
		server_key = DEF_SERVER_KEY;
	
	fburl = FCM_SEND;
	
	if (a_imei_field_offset->count)
		imei_field_offset = *a_imei_field_offset->ival;
	else
		imei_field_offset = DEF_IMEI_FIELD_OFFSET;

	if (a_imei_field_size->count)
		imei_field_size = *a_imei_field_size->ival;
	else
		imei_field_size = DEF_IMEI_FIELD_SIZE;

	if (a_packet_size->count)
		packet_size = *a_packet_size->ival;
	else
		packet_size = DEF_PACKET_SIZE;
	
	
	if (a_time_zone->count)
		timezone = *a_time_zone->ival;
	else
		timezone = DEF_TIME_ZONE;
	
	if (a_test_data_hex->count)
		test_data_hex = *a_test_data_hex->sval;
	else
		test_data_hex = "";

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
