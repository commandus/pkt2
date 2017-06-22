#ifndef HANDLER_PQ_CONFIG_H
#define HANDLER_PQ_CONFIG_H     1

#include <string>
#include <vector>
#include <stdint.h>
#include <libpq-fe.h>

#define PROGRAM_NAME             "pkt2dumppq"
#define PROGRAM_DESCRIPTION      "PostgreSQL packet dumper"

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
	bool create_table;
	int error();

	int retries;             ///< default 1
	int retry_delay;         ///<

	uint32_t verbosity;

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
	int accept_socket;
	
	char *path;
};

/**
 * Establish configured database connection
 */
PGconn *dbconnect(Config *config);

#endif
