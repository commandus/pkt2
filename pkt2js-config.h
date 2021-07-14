#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

#define PROGRAM_NAME             "pkt2js"
#define PROGRAM_DESCRIPTION      "PKT2 to JSON"

/**
 * @brief Parse command line options
 * @param argc command count
 * @param argv command lines
 * @see pktreceiver.cpp
 */
class Config
{
private:

	int lastError;
	/**
	 * @brief Parse command line
	 * 
	 **/
	int parseCmd
	(
		int argc,
		char* argv[]
	);
public:

	Config
	(
		int argc,
		char* argv[]
	);

	/**
	 * @return 
	 * 	0- success
	 * 	1- show help and exit, or command syntax error
	 * 	2- output file does not exists or can not open to write
	 */
	int error();

	uint32_t verbosity;
	size_t buffer_size;

	std::string filenameInput;
	std::string filenameOutput;
	int output_mode;						///< 0- json, 1- hex string, 2- binary
	int input_mode;					///< 0- binary, 1- hex string
	std::string proto_path;			///< proto files directory path
	std::string force_message;

	std::vector <int> allowed_packet_sizes;
	int retries;					///< default 0
	int retry_delay;				///< default 60 seconds

	bool daemonize;
	int max_fd;						///< 0- use default max file descriptor count per process
	int stop_request;				///< 0- process, 1- stop request, 2- reload request
	
	uint64_t session_id;
	uint64_t count_packet_in;
	uint64_t count_packet_out;
	char *path;

	std::map<std::string, std::string> tableAliases;
	std::map<std::string, std::string> fieldAliases;
	std::map<std::string, std::string> properties;
};


#endif
