#ifndef HANDLER_LINE_CONFIG_H
#define HANDLER_LINE_CONFIG_H     1

#include <string>
#include <vector>

#define PROGRAM_NAME             "handlerline"
#define PROGRAM_DESCRIPTION      "PKT2 stdout printer"

/**
  * @brief handlerline command line options
  * @see handlerline.cpp
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

	// proto
	std::string proto_path;	///< proto files directory path
	int mode;		        ///< default 0- JSON
	int format_number;		///< which format to use

	int buffer_size;        ///< default 2048

	bool daemonize;
	int max_fd;				///< 0- use default max file descriptor count per process
	int stop_request;
	int verbosity;          ///< default 0
	
	int accept_socket;
};


#endif
