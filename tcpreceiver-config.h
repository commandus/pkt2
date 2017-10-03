#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <vector>
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
	std::string intface;				///< default 0.0.0.0
	uint32_t    port;					///< default 50052
	uint32_t    verbosity;
	size_t buffer_size;

	int retries;						///< default 0
	int retry_delay;					///< default 60 seconds
	
	// decompression
	int compression_type;				///< default 0- none, 1- modified Huffman, 2- Huffman
	std::string escape_code;			///< Escape code(binary code). default none. If assigned, followed 8 bits is byte itself(not a Huffman code)
	int compression_offset;				///< offset where data compression is started
	std::string frequence_file;			///< huffman frequnces
	std::string codemap_file;			///< huffman code dictionary
	std::vector<size_t> valid_sizes;	///< empty- do not control size of decompressed pacxet. If size if not in the list, try get packet as-is (w/o decompresson).

	bool daemonize;
	int max_fd;				///< 0- use default max file descriptor count per process
	int stop_request;		///< 0- process, 1- stop request, 2- reload request
	std::string message_url;///< ipc:///tmp/packet.pkt2
	
	char *path;	

};

#endif // ifndef CONFIG_H
