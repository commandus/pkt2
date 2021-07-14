#ifndef PKT2GATEWAY_CONFIG_H
#define PKT2GATEWAY_CONFIG_H     1

#include <string>
#include <map>

#define PROGRAM_NAME			"pkt2gateway"
#define PROGRAM_DESCRIPTION		"Send packets from command line or binary file"

#define MODE_RAW				"raw"
#define MODE_HEX				"hex"

/**
  *     @brief pkt2gateway configuration
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
	int retries;					///< default 0
	int retry_delay;				///< default 60 seconds

	std::string message_out_url;
	std::string packet;
	std::string file_name;
	std::string mode;				///< "raw", "hex", <Packet.message>
	std::string proto_path;			///< proto files directory path
	std::string force_message;

	bool daemonize;
	int stop_request;				///< 0- process, 1- stop request, 2- reload request
	size_t buffer_size;
	
	std::map<std::string, std::string> tableAliases;
	std::map<std::string, std::string> fieldAliases;
	std::map<std::string, std::string> properties;
};


#endif
