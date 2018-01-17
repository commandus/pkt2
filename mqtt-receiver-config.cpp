#include <sstream>
#include <limits.h>
#include <unistd.h>
#include <argtable2.h>

#include "mqtt-receiver-config.h"

#define DEF_TOPIC					"pkt2"
#define DEF_PORT					1883
#define DEF_PORT_S					"1883"
#define DEF_ADDRESS					"tcp://127.0.0.1"
#define DEF_QUEUE					"ipc:///tmp/packet.pkt2"
#define DEF_QOS						1
#define DEF_QOS_S					"1"
#define DEF_CLIENT_ID				"mqtt-receiver"
#define DEF_KEEP_ALIVE_INTERVAL		20
#define DEF_KEEP_ALIVE_INTERVAL_S	"20"
#define DEF_RECONNECT_DELAY			5
#define DEF_RECONNECT_DELAY_S		"5"

Config::Config
(
    int argc, 
    char* argv[]
)
{
	stop_request = 0;
	lastError = parseCmd(argc, argv);
}

int Config::error() 
{
	return lastError;
}

std::string Config::getBrokerAddress()
{
	std::stringstream ss;
	ss << broker_address << ":" << broker_port;
	return ss.str();
}

/**
 * Parse command line into struct ClientConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int Config::parseCmd
(
	int argc,
	char* argv[]
)
{
	struct arg_str *a_topic = arg_strn("t", "topic", "<MQTT topic>", 0, 32, "MQTT topic name. Default " DEF_TOPIC);
	struct arg_str *a_broker_address = arg_str0("a", "broker", "<host name/address>", "MQTT broker address. Default " DEF_ADDRESS);
	struct arg_int *a_broker_port = arg_int0("p", "port", "<number>", "MQTT broler TCP port. Default " DEF_PORT_S);
	struct arg_int *a_qos = arg_int0("q", "qos", "<0..2>", "0- at most once, 1- at least once, 2- exactly once. Default " DEF_QOS_S);
	struct arg_str *a_client_id = arg_str0("c", "client", "<id>", "MQTT client identifier string. Default " DEF_CLIENT_ID);
	struct arg_int *a_keep_alive_interval = arg_int0("k", "keepalive", "<seconds>", "Keep alive interval. Default " DEF_KEEP_ALIVE_INTERVAL_S);
	struct arg_int *a_reconnect_delay = arg_int0("w", "reconnect", "<seconds>", "Reconnect interval. Default " DEF_RECONNECT_DELAY_S);

	struct arg_str *a_message_url = arg_str0("o", "output", "<bus url>", "Default ipc:///tmp/packet.pkt2");
	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_int *a_max_fd = arg_int0(NULL, "maxfd", "<number>", "Set max file descriptors. 0- use default (1024).");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
		a_topic, a_broker_address, a_broker_port, a_qos, a_client_id, a_keep_alive_interval, a_reconnect_delay,
		a_message_url, 
		a_retries, a_retry_delay,
		a_daemonize, a_max_fd, a_verbosity,
		a_help, a_end 
	};

	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	// Parse the command line as defined by argtable[]
	nerrors = arg_parse(argc, argv, argtable);

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)
	{
		if (nerrors)
			arg_print_errors(stderr, a_end, PROGRAM_NAME);
		printf("Usage: %s\n", PROGRAM_NAME);
		arg_print_syntax(stdout, argtable, "\n");
		printf("%s\n", PROGRAM_DESCRIPTION);
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	// MQTT
	if (a_broker_address->count)
		broker_address = *a_broker_address->sval;
	else
		broker_address = DEF_ADDRESS;

	if (a_broker_port->count)
		broker_port = *a_broker_port->ival;
	else
		broker_port = DEF_PORT;

	if (a_qos->count)
		qos = *a_qos->ival;
	else
		qos = DEF_QOS;

	if (a_topic->count)
	{
		for (int i = 0; i < a_topic->count; i++)
			topics.push_back(a_topic->sval[i]);
	}
	else
		topics.push_back(DEF_TOPIC);
	
	if (a_client_id->count)
		client_id = *a_client_id->sval;
	else
		client_id = DEF_CLIENT_ID;

	if (a_keep_alive_interval->count)
		keep_alive_interval = *a_keep_alive_interval->ival;
	else
		keep_alive_interval = DEF_KEEP_ALIVE_INTERVAL;

	if (a_reconnect_delay->count)
		reconnect_delay = *a_reconnect_delay->ival;
	else
		reconnect_delay = DEF_RECONNECT_DELAY;
	
	if (a_message_url->count)
		message_url = *a_message_url->sval;
	else
		message_url = DEF_QUEUE;

	if (a_retries->count)
		retries = *a_retries->ival;
	else
		retries = 0;

	if (a_retry_delay->count)
		retry_delay = *a_retry_delay->ival;
	else
		retry_delay = 60;

	verbosity = a_verbosity->count;
	
	daemonize = a_daemonize->count > 0;

	if (a_max_fd > 0)
		max_fd = *a_max_fd->ival;
	else
		max_fd = 0;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}
