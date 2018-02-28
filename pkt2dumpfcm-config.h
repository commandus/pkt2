#ifndef HANDLER_FCM_CONFIG_H
#define HANDLER_FCM_CONFIG_H     1

#include <string>
#include <vector>
#include <stdint.h>
#include <libpq-fe.h>

#define PROGRAM_NAME             "pkt2dumpfcm"
#define PROGRAM_DESCRIPTION      "Send notification to registered mobile devices over FireBase Cloud Messaging"

/**
  * @brief  configuration
  */
class Config
{
private:
	int lastError;
	/**
	* Parse command line
	* Return 0- success
	*        1- show help and exit, or command syntax error
	*        2- output file does not exists or can not open to write
	**/
	int parseCmd
	(
		int argc,
		char* argv[]
	);
public:
	Config(int argc, char* argv[]);
	bool create_table;
	int error();

	int retries;             ///< default 1
	int retry_delay;         ///<

	uint32_t verbosity;

	// FireBase server key
	std::string server_key;
	int imei_field_offset;
	int imei_field_size;
	int packet_size;

	// PostgreSQL
	std::string dbconn;
	std::string dboptionsfile;
	std::string dbname;
	std::string dbuser;
	std::string dbpassword;
	std::string dbhost;
	std::string dbport;
	std::string dbsocket;
	std::string dbcharset;
	int dbclientflags;

	int buffer_size;
	bool daemonize;
	int max_fd;				///< 0- use default max file descriptor count per process

	int stop_request;
	std::string packet_url;
	
	char *path;
};

/**
 * Establish configured database connection
 */
PGconn *dbconnect(Config *config);

#endif
