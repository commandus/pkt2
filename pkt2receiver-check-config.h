#ifndef PKT2RECEIVER_CHECK_CONFIG_H
#define PKT2RECEIVER_CHECK_CONFIG_H     1

#include <string>
#include <vector>

#define PROGRAM_NAME				"pkt2receiver-check"
#define PROGRAM_DESCRIPTION			"Check pkt2receiver bus state"

/**
  * @brief pkt2receiver-check command line options
  * @see pkt2receiver-check.cpp
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

	std::string control_url; 		///< nano message URL
	int retries;             		///< default 1
	int retry_delay;         		///<

	int buffer_size;
	
	bool daemonize;
	int verbosity;			        ///< default 0
	int stop_request;
	char *path;
};


#endif
