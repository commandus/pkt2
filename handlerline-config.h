#ifndef HANDLER_LINE_CONFIG_H
#define HANDLER_LINE_CONFIG_H     1

#include <string>
#include <vector>
#include <map>
#include <ostream>

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
	std::string file_name;
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
	int stop_request;
	int verbosity;          ///< default 0
	
	std::ostream *stream;
	
	char *path;

	std::map<std::string, std::string> tableAliases;
	std::map<std::string, std::string> fieldAliases;

	int sql_dialect;
};

#endif
