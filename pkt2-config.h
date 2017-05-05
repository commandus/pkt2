#ifndef PKT2_CONFIG_H
#define PKT2_CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "pkt2"
#define PROGRAM_DESCRIPTION      "PKT2 control program"

/**
  *     @brief tcpemitter configuration
  *     @see tcpemitter.cpp
  */
class Config
{
private:
	int lastError;
	/**
	* @brief Parse command line
	* @return 0- success
	**/
	int parseCmd
	(
		int argc,
		char* argv[]
	);
	public:
	Config(int argc, char* argv[]);
	int error();

	int verbosity;

	std::string path;
	std::string file_name;
	bool mode_output;
	bool daemonize;
	int stop_request;		///< 0- process, 1- stop request, 2- reload request
};

#endif
