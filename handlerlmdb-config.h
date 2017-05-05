#ifndef HANDLER_LMDB_CONFIG_H
#define HANDLER_LMDB_CONFIG_H     1

#include <string>
#include <vector>

#define PROGRAM_NAME             "handlerlmdb"
#define PROGRAM_DESCRIPTION      "PKT2 LMDB writer"

/**
  * @see write2lmdb.cpp
  * @see lmdbwritee.cpp
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

	std::string message_url; ///< nano message URL
	std::vector <std::string> allowed_messages;
	int retries;             ///< default 1
	int retry_delay;         ///<

	// LMDB
	std::string path;        ///< path, default "lmdb"
	int mode;                ///< default 0664
	int flags;               ///< default 0

	// proto
	std::string proto_path;	 ///< proto files directory path

	bool daemonize;
	int max_fd;				///< 0- use default max file descriptor count per process
	int stop_request;
	int accept_socket;

	int verbosity;           ///< default 0
	int buffer_size;		 ///< default 4096
};

#endif
