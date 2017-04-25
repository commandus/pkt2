#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <vector>
#include <stdint.h>

#define PROGRAM_NAME             "pkt2receiver"
#define PROGRAM_DESCRIPTION      "PKT2 receiver"

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

	std::string in_url;
	std::string out_url;
	std::string proto_path;			///< proto files directory path
	std::string force_message;

	std::vector <int> allowed_packet_sizes;
	int retries;					///< default 0
	int retry_delay;				///< default 60 seconds

	bool daemonize;
	int max_fd;						///< 0- use default max file descriptor count per process
	int stop_request;				///< 0- process, 1- stop request, 2- reload request
	
	int socket_accept;
};


#endif
