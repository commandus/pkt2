#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <stdint.h>

#define PROGRAM_NAME				"tcpreceiver"
#define PROGRAM_DESCRIPTION			"PKT2 tcp packet listener sends raw packet"

/**
 * @brief tcpreceiver configuration
 * @see tcpreceiver.cpp
 */
class Config {
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
		char *argv[]
	);

public:

	Config
	(
		int   argc,
		char *argv[]
	);
	int error();
	std::string intface;	///< default 0.0.0.0
	uint32_t    port;		///< default 50052
	uint32_t    verbosity;
	size_t buffer_size;

	int retries;			///< default 0
	int retry_delay;		///< default 60 seconds

	bool daemonize;
	int  max_fd;			///< 0- use default max file descriptor count
							//per process
	bool stop_request;
	std::string message_url;///< ipc:///tmp/packet.pkt2
};

#endif // ifndef CONFIG_H
