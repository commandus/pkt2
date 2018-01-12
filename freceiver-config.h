#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <stdint.h>

#define PROGRAM_NAME				"freceiver"
#define PROGRAM_DESCRIPTION			"PKT2 file reader. Fixed packet size."
#define DEF_PROTO_PATH				"proto"

#define FILE_MODE_BIN				0
#define FILE_MODE_TEXT_HEX			1
#define FILE_MODE_TEXT_INT			2

/**
 * @brief freceiver configuration
 * @see freceiver.cpp
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
	int file_mode;				///< 0- binary, 1- eol text hex, 2- decimal line by line
	int packet_size;			///< word size in bytes or precision
	std::string filename_in;
	int mode;					///< -1- as usual. 0- JSON, CSV- 1, TAB- 2, SQL- 3, SQL2- 4, Protobuf text- 5, DEBUG-6

	std::string proto_path;		///< proto files directory path
	uint32_t    verbosity;
	size_t buffer_size;

	int retries;				///< default 0
	int retry_delay;			///< default 60 seconds

	bool daemonize;
	int max_fd;					///< 0- use default max file descriptor count per process
	int stop_request;			///< 0- process, 1- stop request, 2- reload request
	std::string message_url;	///< default ipc:///tmp/packet.pkt2
	
	char *path;	

};

#endif // ifndef CONFIG_H
