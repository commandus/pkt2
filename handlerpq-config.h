#ifndef HANDLER_PQ_CONFIG_H
#define HANDLER_PQ_CONFIG_H     1

#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <libpq-fe.h>

#define PROGRAM_NAME             "handlerpq"
#define PROGRAM_DESCRIPTION      "PostgreSQLreceiver"

/**
  * @brief  
  * @see
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
	int error();

	int retries;             ///< default 1
	int retry_delay;         ///<

	uint32_t verbosity;

	// Protobuf
	std::string proto_path;
	std::vector <std::string> allowed_messages;
	
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
	int mode;				///< default 4- SQL(2)
	int format_number;		///< which format to use
	bool daemonize;

	int stop_request;
	std::string message_url;
	
	char *path;

	std::map<std::string, std::string> tableAliases;
	std::map<std::string, std::string> fieldAliases;
};

/**
 * Establish configured database connection
 */
PGconn *dbconnect(Config *config);

#endif
