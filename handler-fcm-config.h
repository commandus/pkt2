#ifndef HANDLER_FCM_CONFIG_H
#define HANDLER_FCM_CONFIG_H     1

#include <string>
#include <vector>
#include <postgresql/libpq-fe.h>

#define PROGRAM_NAME             	"handlerfcm"
#define PROGRAM_DESCRIPTION			"FireBase Cloud messaging printer"
#define FCM_SEND					"https://fcm.googleapis.com/fcm/send"

void ontokenbearer
(
	void *env,
	const std::string &value,
	int status
);

/**
  * @brief google-sheets-writer command line options
  * @see google-sheets-writer.cpp
  */
class Config
{
private:
	int lastError;
	/**
	* Parse command line
	* Return 0- success
	**/
	int parseCmd
	(
		int argc,
		char* argv[]
	);
public:
	Config(int argc, char* argv[]);
	int error();
	
	std::string fburl;
	std::string message_url; 		///< nano message URL
	std::string imei_field_name; 		///< field name contains IMEI, default "imei"
	int retries;             		///< default 1
	int retry_delay;         		///<

	// proto
	std::string connection;			///< proto files directory path
	int buffer_size;        		///< default 2048

	bool daemonize;
	int stop_request;

	int verbosity;			        ///< default 0
	int format_number;		        ///< default 0
	char *path;
	std::string proto_path;
	std::vector <std::string> allowed_messages;
	
	// FireBase server key
	std::string server_key;
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
};

	/**
	* Establish configured database connection
	*/
	PGconn *dbconnect(Config *config);

#endif
