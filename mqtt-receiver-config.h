#ifndef CONFIG_H
#define CONFIG_H					1

#include <string>
#include <vector>
#include <stdint.h>

#define PROGRAM_NAME				"mqtt-receiver"
#define PROGRAM_DESCRIPTION			"MQTT subscriber client receive packet and send back raw packet to the packet queue"

/**
 * @brief mqtt-receiver configuration
 * @see mqtt-receiver.cpp
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

	std::string getBrokerAddress();
	std::string broker_address;
	std::string client_id;
	uint32_t broker_port;				///< default 1883
	uint32_t keep_alive_interval;		///< default 20
	std::vector<std::string> topics;
	uint32_t qos;						///< default 1- At least once: The message is always delivered at least once.
	uint32_t verbosity;

	int retries;						///< default 0
	int retry_delay;					///< default 60 seconds
	uint32_t reconnect_delay;			///< default 60s
	
	bool daemonize;
	int max_fd;				///< 0- use default max file descriptor count per process
	int stop_request;		///< 0- process, 1- stop request, 2- reload request
	std::string message_url;///< ipc:///tmp/packet.pkt2
};

#endif // ifndef CONFIG_H
