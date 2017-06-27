#ifndef REPEATOR_CONFIG_H
#define REPEATOR_CONFIG_H     1

#include <string>
#include <vector>

#define PROGRAM_NAME				"pkt2receiver-check"
#define PROGRAM_DESCRIPTION			"bus repeator receive from bus and send to other(s)"

/**
  * @brief repeator command line options
  * @see repeator.cpp
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
	virtual ~Config();
	int error();

	std::string in_url;		 		///< nano message URL
	std::vector<std::string> out_urls;	 		///< nano message URL
	int retries;             		///< default 1
	int retry_delay;         		///<

	int buffer_size;
	
	bool daemonize;
	int verbosity;			        ///< default 0
	int in_socket;
	std::vector<int> out_sockets;
	int stop_request;
	char *path;
};

#endif
