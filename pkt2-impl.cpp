/**
 * Read message types and data as json line per line.
 * Line format is:
 *   Packet.MessageType:{"json-object-in-one-line"}
 */
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <vector>

#include <glog/logging.h>

#include "duk/duktape.h"

#include "pkt2-impl.h"
#include "pkt2-config.h"
#include "utilstring.h"
#include "errorcodes.h"

/**
 * @brief start program
 * @param cmd program path
 * @param retval procress identifier
 * @return 0- success
 */
int start_program
(
	const std::string &path,
	const std::string &cmd,
	const std::vector<std::string> &args,
	pid_t *retval
)
{
	*retval = fork(); // create child process
	switch (*retval)
	{
	case -1: 
		// fork error
		return ERRCODE_FORK;
	case 0: 
		{
			const std::string &fp(path + "/" + cmd);
			const std::string &lib("LD_LIBRARY_PATH=" + path + "/lib");
			// run the command in the child process
			char *const env[] = { (char *) lib.c_str(), (char *) 0 };
			char **argv  = (char **) malloc(args.size() * sizeof(char *));
			for (int i = 0; i < args.size(); i++)
			{
				argv[i] = (char*) args[i].c_str();
			}
			execve(fp.c_str(), argv, env);
			// exec doesn't return unless there is a problem
			free(argv);
		}
		return ERRCODE_EXEC; 
	}
	return 0;
}

/**
 * @brief Check is process is is_running
 * @param pid process identifier
 * @return true- process running
 */
int stop_process
(
	pid_t &pid
)
{
	return kill(pid, SIGTERM);
}

/**
 * @brief Check is process is is_running
 * @param pid process identifier
 * @return true- process running
 */
bool is_process_running
(
	pid_t &pid
)
{
	int r = kill(pid, 0);
	return (r == 0);
}

class ProcessDescriptor
{
public:
	pid_t pid;
	std::string path;
	std::string cmd;
	std::vector<std::string> args;
	
	ProcessDescriptor(
		const std::string &a_path,
		const std::string &a_cmd,
		std::vector<std::string> a_args
	)
	: pid(0), path(a_path), cmd(a_cmd), args(a_args)
	{
		start();
	}

	~ProcessDescriptor()
	{
		stop();
	}

	int start()
	{
		return start_program(path, cmd, args, &pid);
	}
	
	int stop()
	{
		return stop_process(pid);
	}
	
	bool is_running()
	{
		return (pid != 0) && is_process_running(pid);
	}
};

/**
 * @brief Javascript error handler
 * @param env JavascriptContext object
 * @param msg error message
 * 
 */
void duk_fatal_handler(
	void *env, 
	const char *msg
)
{
	fprintf(stderr, "Javascript error: %s\n", (msg ? msg : ""));
	fflush(stderr);
	abort();
}

/**
 *	tcp_listeners = 
[
	{
		"port": 50052,
		"ip": "0.0.0.0"
	}
];
*/
class CfgListenerTcp
{
public:
	std::string ip;
	int port;
	CfgListenerTcp() : ip("127.0.0.1"), port(50052) {};
};

/**
 * 		mqtt_listeners = 
[
	{
		"client": "cli01",
		"broker": "tcp://127.0.0.1",
		"port": 1883,
		"qos": 1, 
		"keep-alive": 20
	}
];
*/
class CfgListenerMqtt
{
public:
	std::string client;
	std::string broker;
	int port;
	int qos;
	int keep_alive;
	CfgListenerMqtt() : client(""), broker("tcp://localhost"), port(1883), qos(1), keep_alive(20) {};
};

/**
 * 	packet2message = 
[
	{
		"sizes": 
		[
			81
		],
		"force-message": "iridium.animals"
	}
];

*/
class CfgPacket2Message
{
public:
	std::string force_message;
	std::vector <int> sizes;
	CfgPacket2Message() : force_message("") {};
};

/**
 * write_file = 
[
	{
		"messages":
		[
			"iridium.animals"
		],
		"mode": 0,
		"file": "/dev/null"
	}
];
*/
class CfgWriteFile
{
public:
	int mode;
	std::string file;
	std::vector <std::string> messages;
	CfgWriteFile() : mode(0), file("") {};
};

/**
 * write_lmdb = 
[
	{
		"messages":
		[
			"iridium.animals"
		],
		"dbpath": "/dev/null"
	}
];
*/
class CfgWriteLmdb
{
public:
	std::string dbpath;
	std::vector <std::string> messages;
	CfgWriteLmdb() : dbpath("") {};
};

/**
 * write_pq = 
[
	{
		"user": "pkt2",
		"password": "pkt2",
		"scheme": "pkt2",
		"host": "localhost",
		"port": 5432
	}
];
*/
class CfgWritePq
{
public:
	std::string user;
	std::string password;
	std::string scheme;
	std::string host;
	int port;
	std::vector <std::string> messages;
	CfgWritePq() : user(""), password(""), scheme(""), host("localhost"), port(5432) {};
};

/**
 * write_google_sheets = 
[
	{
		"json_service_file_name": "cert/pkt2-sheets.json",
		"bearer_file_name": ".token-bearer.txt",
		"email": "andrei.i.ivanov@commandus.com",
		"spreadsheet": "1iDg77CjmqyxWyuFXZHat946NeAEtnhL6ipKtTZzF-mo",
		"sheet": "Temperature"
	}
];
*/
class CfgWriteGoogleSheets
{
public:
	std::string json_service_file_name;
	std::string bearer_file_name;
	std::string email;
	std::string spreadsheet;
	std::string sheet;
	std::vector <std::string> messages;
	CfgWriteGoogleSheets() : json_service_file_name(""), bearer_file_name(".token-bearer.txt"), email(""), spreadsheet(""), sheet("") {};
};

class ProcessDescriptors
{
private:
	duk_context *context;
	
public:
	std::string proto_path;
	std::string bus_in;
	std::string bus_out;
	int max_file_descriptors;
	int max_buffer_size;

	std::vector<CfgListenerTcp> cfgListenerTcp;
	std::vector<CfgListenerMqtt> cfgListenerMqtt;
	std::vector<CfgPacket2Message> cfgPacket2Message;
	std::vector<CfgWriteFile> cfgWriteFile;
	std::vector<CfgWriteLmdb> cfgWriteLmdb;
	std::vector<CfgWritePq> cfgWritePq;
	std::vector<CfgWriteGoogleSheets> cfgWriteGoogleSheets;
	
	std::vector<ProcessDescriptor> descriptors;
	
	ProcessDescriptors()
	{
		context = duk_create_heap(NULL, NULL, NULL, this, duk_fatal_handler);
	}

	void load_globals() 
	{
		duk_push_global_object(context);
		
		duk_get_prop_string(context, -1, "proto_path");
		if (duk_is_string(context, -1)) 
			proto_path = duk_get_string(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "bus_in");
		if (duk_is_string(context, -1)) 
			bus_in = duk_get_string(context, -1);
		duk_pop(context); 

		duk_get_prop_string(context, -1, "bus_out");
		if (duk_is_string(context, -1)) 
			bus_out = duk_get_string(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "max_file_descriptors");
		if (duk_is_number(context, -1)) 
			max_file_descriptors = duk_get_number(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "max_buffer_size");
		if (duk_is_number(context, -1)) 
			max_buffer_size = duk_get_number(context, -1);
		duk_pop(context);

		// read listener settings
		// tcp 
		duk_get_prop_string(context, -1, "tcp_listeners");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgListenerTcp cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "ip"))
						cfg.ip = duk_get_string(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "port"))
						cfg.port = duk_get_number(context, -1);
					duk_pop(context);
				} 
				duk_pop(context);
				cfgListenerTcp.push_back(cfg);
			}
		}
		duk_pop(context);

		// MQTT
		duk_get_prop_string(context, -1, "mqtt_listeners");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgListenerMqtt cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "client"))
						cfg.client = duk_get_string(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "broker"))
						cfg.broker = duk_get_string(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "port"))
						cfg.port = duk_get_number(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "qos"))
						cfg.qos = duk_get_number(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "keep-alive"))
						cfg.keep_alive = duk_get_number(context, -1);
					duk_pop(context);
				} 
				duk_pop(context);
				cfgListenerMqtt.push_back(cfg);
			}
		}
		duk_pop(context);
		
		// packet2message
		duk_get_prop_string(context, -1, "packet2message");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgPacket2Message cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "force-message"))
						cfg.force_message = duk_get_string(context, -1);
					duk_pop(context);
					
					duk_get_prop_string(context, -1, "sizes");
					if (duk_is_array(context, -1)) 
					{
						
						duk_size_t sn = duk_get_length(context, -1);
						
						for (duk_size_t s = 0; s < sn; s++) 
						{
							if (duk_get_prop_index(context, -1, s)) 
							{
								cfg.sizes.push_back(duk_get_number(context, -1));
							}
							duk_pop(context);
						}
						
					}
					duk_pop(context);
					
				} 
				duk_pop(context);
				cfgPacket2Message.push_back(cfg);
			}
		}
		duk_pop(context); // duk_get_prop_string
		
		// write file
		duk_get_prop_string(context, -1, "write_file");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgWriteFile cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "file"))
						cfg.file = duk_get_string(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "mode"))
						cfg.mode = duk_get_number(context, -1);
					duk_pop(context);
					
					duk_get_prop_string(context, -1, "messages");
					if (duk_is_array(context, -1)) 
					{
						duk_size_t sn = duk_get_length(context, -1);
						for (duk_size_t s = 0; s < sn; s++) 
						{
							if (duk_get_prop_index(context, -1, s)) 
							{
								cfg.messages.push_back(duk_get_string(context, -1));
							}
							duk_pop(context);
						}
					}
					duk_pop(context);
				} 
				duk_pop(context);
				cfgWriteFile.push_back(cfg);
			}
		}
		duk_pop(context); // duk_get_prop_string

		// write LMDB
		duk_get_prop_string(context, -1, "write_lmdb");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgWriteLmdb cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "dbpath"))
						cfg.dbpath = duk_get_string(context, -1);
					duk_pop(context);
					
					duk_get_prop_string(context, -1, "messages");
					if (duk_is_array(context, -1)) 
					{
						duk_size_t sn = duk_get_length(context, -1);
						for (duk_size_t s = 0; s < sn; s++) 
						{
							if (duk_get_prop_index(context, -1, s)) 
							{
								cfg.messages.push_back(duk_get_string(context, -1));
							}
							duk_pop(context);
						}
					}
					duk_pop(context);
				} 
				duk_pop(context);
				cfgWriteLmdb.push_back(cfg);
			}
		}
		duk_pop(context); // duk_get_prop_string

		// write PostgreSQL
		duk_get_prop_string(context, -1, "write_pq");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgWritePq cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "user"))
						cfg.user = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "password"))
						cfg.password = duk_get_string(context, -1);
					duk_pop(context);
					
					if (duk_get_prop_string(context, -1, "scheme"))
						cfg.scheme = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "host"))
						cfg.host = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "port"))
						cfg.port = duk_get_number(context, -1);
					duk_pop(context);

					duk_get_prop_string(context, -1, "messages");
					if (duk_is_array(context, -1)) 
					{
						duk_size_t sn = duk_get_length(context, -1);
						for (duk_size_t s = 0; s < sn; s++) 
						{
							if (duk_get_prop_index(context, -1, s)) 
							{
								cfg.messages.push_back(duk_get_string(context, -1));
							}
							duk_pop(context);
						}
					}
					duk_pop(context);
				} 
				duk_pop(context);
				cfgWritePq.push_back(cfg);
			}
		}
		duk_pop(context); // duk_get_prop_string

		// write Google Sheets
		duk_get_prop_string(context, -1, "write_google_sheets");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgWriteGoogleSheets cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "json_service_file_name"))
						cfg.json_service_file_name = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "bearer_file_name"))
						cfg.bearer_file_name = duk_get_string(context, -1);
					duk_pop(context);
					
					if (duk_get_prop_string(context, -1, "email"))
						cfg.email = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "spreadsheet"))
						cfg.spreadsheet = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "sheet"))
						cfg.sheet = duk_get_string(context, -1);
					duk_pop(context);

					duk_get_prop_string(context, -1, "messages");
					if (duk_is_array(context, -1)) 
					{
						duk_size_t sn = duk_get_length(context, -1);
						for (duk_size_t s = 0; s < sn; s++) 
						{
							if (duk_get_prop_index(context, -1, s)) 
							{
								cfg.messages.push_back(duk_get_string(context, -1));
							}
							duk_pop(context);
						}
					}
					duk_pop(context);
				} 
				duk_pop(context);
				cfgWriteGoogleSheets.push_back(cfg);
			}
		}
		duk_pop(context); // duk_get_prop_string

	}
	
	~ProcessDescriptors()
	{
		duk_pop(context);
		duk_destroy_heap(context);
	}
	
	int load
	(
		const std::string &path,
		const std::string &config_file_name
	)
	{
		std::string v = file2string(config_file_name);
		duk_eval_string(context, v.c_str());
		duk_pop(context);  // ignore result
		load_globals();
		return 0;
	}
	
	int start()
	{
		for (int i = 0; i < descriptors.size(); i++)
		{
			descriptors[i].start();
		}
		return 0;
	}
	
	int check()
	{
		for (int i = 0; i < descriptors.size(); i++)
		{
			if (!descriptors[i].is_running())
				descriptors[i].start();
		}
		return 0;
	}
	
	int stop()
	{
		for (int i = 0; i < descriptors.size(); i++)
		{
			descriptors[i].stop();
		}
		return 0;
	}
};

/**
 * @brief send loop
 * @param config Configuration
 * @return 0- OK
 */
int pkt2
(
	Config *config
)
{
	ProcessDescriptors processes;
START:
	if (processes.load(".", config->file_name) != ERR_OK)
		return ERRCODE_CONFIG;
	processes.start();
	config->stop_request = 0;
	while (!config->stop_request)
	{
		processes.check();
		sleep(1);
	}
	processes.stop();
	
	if (config->stop_request == 2)
		goto START;
	return ERR_OK;
}

/**
 * @brief stop
 * @param config Configuration
 * @return 0- success
 *        1- config is not initialized yet
 */
int stop(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	config->stop_request = 1;
	return ERR_OK;
}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	return ERR_OK;
}