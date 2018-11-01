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
#include <dirent.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <vector>

#include <glog/logging.h>

#include "platform.h"
#include "duk/duktape.h"

#include "pkt2-impl.h"
#include "utilstring.h"
#include "errorcodes.h"

#define	DEF_SERVER_KEY	"AAAAITL4VBA:APA91bGQwuvaQTt8klgebh8QO1eSU7o5itF0QGnp7kCWJNgMwe8WM3bMh6eGDkeyMbvUAmE2MqtB1My3f0-mHM6MQE1gOjMB0eiAW1Xaqds0hYETRNzqAe0iRh5v-PcxmxrHQeJh6Nuj"

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
			char *const env[] = { (char *) lib.c_str(), (char *) 0 };	// strdup()?
			char **argv = (char **) malloc((args.size() + 2) * sizeof(char *));
			argv[0] = (char *) fp.c_str();								// strdup()?
			for (int i = 0; i < args.size(); i++)
			{
				argv[i + 1] = (char*) args[i].c_str();					// strdup()?
			}
			argv[args.size() + 1] = (char *) 0;
			// if (execve(fp.c_str(), argv, env) == -1)
			if (execv(fp.c_str(), argv) == -1)
			{
				LOG(ERROR) << fp << " error " << errno << ": " << strerror(errno);
				kill(*retval, SIGKILL);
				exit(errno);
			}
			// exec doesn't return unless there is a problem
			free(argv);
		}
		return ERRCODE_EXEC; 
	}
	return 0;
}

/**
 * @brief Stop process
 * @param pid process identifier
 * @return 0- process stopped
 */
int stop_process
(
	pid_t &pid
)
{
	if (pid)
	{
		return kill(pid, SIGINT);
	}
	else
		return 0;
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
	if (pid == 0)
		return true;
	DIR *dir;
	if (!(dir = opendir(("/proc/" + toString(pid)).c_str()))) 
        return false;
	closedir(dir);
	
	// open the cmdline file
	std::string fn("/proc/" + toString(pid) + "/cmdline");
	FILE* fp = fopen(fn.c_str(), "r");
	bool ok = false;
	if (fp) 
	{
		char buf[128];
		ok = fgets(buf, sizeof(buf), fp) != NULL;
		fclose(fp);
	}
    return ok;
}

class ProcessDescriptor
{
public:
	pid_t pid;
	/// working directory (where binary resides)
	std::string path;
	/// executable file name
	std::string cmd;
	/// command line arguments
	std::vector<std::string> args;
	/// start count
	int start_count;
	/// last time started
	time_t last_start;
	
	ProcessDescriptor(
		const std::string &a_path,
		const std::string &a_cmd,
		std::vector<std::string> a_args
	)
	: pid(0), path(a_path), cmd(a_cmd), args(a_args), start_count(0), last_start(0)
	{
		// start();
	}

	~ProcessDescriptor()
	{
		// stop();
	}

	int start()
	{
		int r = start_program(path, cmd, args, &pid);
		if (r == 0)	
		{
			start_count++;
			last_start = time(NULL);
		}
		return r;
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
void duk_fatal_handler_process_descriptors(
	void *env, 
	const char *msg
)
{
	fprintf(stderr, "Javascript error: %s\n", (msg ? msg : ""));
	fflush(stderr);
	abort();
}

class CfgCommon
{
public:
	std::string proto_path;
	std::string bus_in;
	std::string bus_out;
	int max_file_descriptors;
	int max_buffer_size;
	int verbosity;

	CfgCommon() 
		: proto_path("proto"), bus_in("ipc:///tmp/packet.pkt2"), bus_out("ipc:///tmp/message.pkt2"), 
		max_file_descriptors(0), max_buffer_size(0), verbosity(0) {};
};

#define PUSH_BACK_ARG_LIT(r, a) 		r.push_back(a)
#define PUSH_BACK_ARG_STR(r, a, s) 		r.push_back(a); r.push_back(s)
#define PUSH_BACK_ARG_NUM(r, a, v) 		r.push_back(a); r.push_back(toString(v))
/**
 * 
 * - compression_type	По умолчанию 0. 1- сжатие (Huffman). Для сжатия нудно указать или файл частот для построения таблицы кодов или готовую таблицу кодов 
 * - escape_code Специальный экранирующий код- префикс(escape- код), который используется в качестве префикса для следующих за ним восьми бит значения. 
 * По умолчанию 0(не задан). Имеет значение, если compression_type	больше 0.
 * - compression_offset Смещение в байтах, начиная с которого данные сжимаются. По умолчанию 0. Имеет значение, если compression_type	больше 0.
 * - frequence_file	Файл частот, используемый для построения таблицы кодов Хаффмана. Имеет значение, если compression_type больше 0 и не задан параметр codemap_file.
 * - codemap_file	 Готовая таблицы кодов Хаффмана. Имеет значение, если compression_type	больше 0. 
 * - valid_sizes: [] Допустимые размеры пакета после распаковки. Если не указано ни одного размера, проверка на размер после распаковки не делается. 
 * Если указан хотя бы один размер, то если после распаквки размер пакета не равен олдноу из перечисленных, распаквка отменяется, и пакет передается как есть.
 *
 *	tcp_listeners = 
 *	[
 *	{
 *		"port": 50052,
 *		"ip": "0.0.0.0"
 *		compression_type: 0,
 *		escape_code: "",
 *		compression_offset: 0,
 *		frequence_file: "",
 *		codemap_file: "",
 *		valid_sizes: []
 *}
 *];
 * 
*/
class CfgListenerTcp
{
public:
	std::string ip;
	int port;
	int compression_type;
	std::string escape_code;
	int compression_offset;
	std::string frequence_file;
	std::string codemap_file;
	std::vector<int> valid_sizes;

	CfgListenerTcp() 
	: ip("0.0.0.0"), port(50052),
		compression_type(0), escape_code(""), compression_offset(0), 
		frequence_file(""), codemap_file("")
	{
		
	};
	
	std::vector<std::string> args(CfgCommon *common) 
	{
		std::vector<std::string> r;
		if (ip != "0.0.0.0")
		{
			PUSH_BACK_ARG_STR(r, "-i", ip);
		}
		
		if (port != 50052)
		{
			PUSH_BACK_ARG_NUM(r, "-l", port);
		}

		if (compression_type)
		{
			PUSH_BACK_ARG_NUM(r, "-c", compression_type);
		}

		if (escape_code.empty())
		{
			PUSH_BACK_ARG_STR(r, "-e", escape_code);
		}

		if (compression_offset)
		{
			PUSH_BACK_ARG_NUM(r, "-s", compression_offset);
		}
		
		if (!frequence_file.empty())
		{
			PUSH_BACK_ARG_STR(r, "-f", frequence_file);
		}

		if (!codemap_file.empty())
		{
			PUSH_BACK_ARG_STR(r, "-m", codemap_file);
		}

		for (int i = 0; i < valid_sizes.size(); i++)
		{
			PUSH_BACK_ARG_NUM(r, "-p", valid_sizes[i]);
		}

		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_in != "ipc:///tmp/packet.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-o", common->bus_in);
			}
			if (common->max_file_descriptors)
			{
				PUSH_BACK_ARG_NUM(r, "--maxfd", common->max_file_descriptors);
			}
			if (common->max_buffer_size)
			{
				if (common->max_buffer_size != 4096)
				{
					PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
				}
			}
		}
		return r;
	};
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
	std::string topic;
	int port;
	int qos;
	int keep_alive;
	CfgListenerMqtt() : client(""), broker("tcp://localhost"), topic(""), port(1883), qos(1), keep_alive(20) {};
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		PUSH_BACK_ARG_STR(r, "-c", client);
		PUSH_BACK_ARG_STR(r, "-a", broker);
		PUSH_BACK_ARG_STR(r, "-t", topic);
		PUSH_BACK_ARG_NUM(r, "-p", port);
		PUSH_BACK_ARG_NUM(r, "-q", qos);
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_in != "ipc:///tmp/packet.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-o", common->bus_in);
			}
			if (common->max_file_descriptors)
			{
				PUSH_BACK_ARG_NUM(r, "--maxfd", common->max_file_descriptors);
			}
		}
		return r;
	};
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
class CfgListenerFile
{
public:
	std::string file;
	int mode;
	CfgListenerFile() : file(""), mode(0) {};
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		PUSH_BACK_ARG_STR(r, "-i", file);
		PUSH_BACK_ARG_NUM(r, "-t", mode);
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_in != "ipc:///tmp/packet.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-o", common->bus_in);
			}
			if (common->max_file_descriptors)
			{
				PUSH_BACK_ARG_NUM(r, "--maxfd", common->max_file_descriptors);
			}
		}
		return r;
	};
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
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		if (!force_message.empty())
		{
			PUSH_BACK_ARG_STR(r, "--message", force_message);
		}
		for (std::vector<int>::const_iterator it(sizes.begin()); it != sizes.end(); ++it)
		{
			PUSH_BACK_ARG_NUM(r, "-a", *it);
		}
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_in != "ipc:///tmp/packet.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-i", common->bus_in);
			}

			if (common->bus_out != "ipc:///tmp/message.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-o", common->bus_out);
			}

			if (!common->proto_path.empty())
			{
				if (common->proto_path != "proto")
				{
					PUSH_BACK_ARG_STR(r, "-p", common->proto_path);
				}
			}
			if (common->max_buffer_size)
			{
				if (common->max_buffer_size != 4096)
				{
					PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
				}
			}
			if (common->max_file_descriptors)
			{
				PUSH_BACK_ARG_NUM(r, "--maxfd", common->max_file_descriptors);
			}

		}
		return r;
	};
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
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		PUSH_BACK_ARG_STR(r, "-o", file);
		PUSH_BACK_ARG_NUM(r, "--format", mode);

		for (std::vector<std::string>::const_iterator it(messages.begin()); it != messages.end(); ++it)
		{
			PUSH_BACK_ARG_STR(r, "-a", *it);
		}
		
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_out != "ipc:///tmp/message.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-i", common->bus_out);
			}
// 			if (!common->proto_path.empty())
			{
				if (common->proto_path != "proto")
				{
					PUSH_BACK_ARG_STR(r, "-p", common->proto_path);
				}
			}
		}
		return r;
	};
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
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		PUSH_BACK_ARG_STR(r, "--dbpath", dbpath);

		for (std::vector<std::string>::const_iterator it(messages.begin()); it != messages.end(); ++it)
		{
			PUSH_BACK_ARG_STR(r, "-a", *it);
		}
		
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_out != "ipc:///tmp/message.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-i", common->bus_out);
			}

			if (!common->proto_path.empty())
			{
				if (common->proto_path != "proto")
				{
					PUSH_BACK_ARG_STR(r, "-p", common->proto_path);
				}
			}
			if (common->max_buffer_size != 4096)
			{
				PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
			}
		}
		return r;
	};
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
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		PUSH_BACK_ARG_STR(r, "--user", user);
		PUSH_BACK_ARG_STR(r, "--password", password);
		PUSH_BACK_ARG_STR(r, "--database", scheme);
		PUSH_BACK_ARG_STR(r, "--host", host);
		if (port != 5432)
		{
			PUSH_BACK_ARG_NUM(r, "--port", port);
		}

		for (std::vector<std::string>::const_iterator it(messages.begin()); it != messages.end(); ++it)
		{
			PUSH_BACK_ARG_STR(r, "-a", *it);
		}

		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_out != "ipc:///tmp/message.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-i", common->bus_out);
			}

			if (!common->proto_path.empty())
			{
				if (common->proto_path != "proto")
				{
					PUSH_BACK_ARG_STR(r, "-p", common->proto_path);
				}
			}
			if (common->max_buffer_size != 4096)
			{
				PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
			}

		}
		return r;
	};
};

/**
 * write_fcm = 
[
	{
		"key": "FCM token",
		"imei": "IMEI",
		"user": "pkt2",
		"password": "pkt2",
		"scheme": "pkt2",
		"host": "localhost",
		"port": 5432
	}
];
*/

class CfgWriteFcm
{
public:
	std::string user;
	std::string password;
	std::string scheme;
	std::string host;
	int port;
	std::vector <std::string> messages;
	std::string imei;
	std::string key;
	CfgWriteFcm() : user(""), password(""), scheme(""), host("localhost"), port(5432), imei(""), key("") {};
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		if ((!key.empty()) && (key != DEF_SERVER_KEY))
			PUSH_BACK_ARG_STR(r, "--key", key);
		if ((!imei.empty()) && (imei == "imei"))
			PUSH_BACK_ARG_STR(r, "--imei", imei);
		PUSH_BACK_ARG_STR(r, "--user", user);
		PUSH_BACK_ARG_STR(r, "--password", password);
		PUSH_BACK_ARG_STR(r, "--database", scheme);
		PUSH_BACK_ARG_STR(r, "--host", host);
		if (port != 5432)
		{
			PUSH_BACK_ARG_NUM(r, "--port", port);
		}

		for (std::vector<std::string>::const_iterator it(messages.begin()); it != messages.end(); ++it)
		{
			PUSH_BACK_ARG_STR(r, "-a", *it);
		}
		
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_out != "ipc:///tmp/message.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-i", common->bus_out);
			}

			if (!common->proto_path.empty())
			{
				if (common->proto_path != "proto")
				{
					PUSH_BACK_ARG_STR(r, "-p", common->proto_path);
				}
			}
			if (common->max_buffer_size != 4096)
			{
				PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
			}

		}
		return r;
	};
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
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		PUSH_BACK_ARG_STR(r, "-g", json_service_file_name);
		PUSH_BACK_ARG_STR(r, "-B", bearer_file_name);
		PUSH_BACK_ARG_STR(r, "-e", email);
		PUSH_BACK_ARG_STR(r, "-s", spreadsheet);
		PUSH_BACK_ARG_STR(r, "-t", sheet);

		for (std::vector<std::string>::const_iterator it(messages.begin()); it != messages.end(); ++it)
		{
			PUSH_BACK_ARG_STR(r, "-a", *it);
		}
		
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->bus_out != "ipc:///tmp/message.pkt2")
			{
				PUSH_BACK_ARG_STR(r, "-i", common->bus_out);
			}

			if (!common->proto_path.empty())
			{
				if (common->proto_path != "proto")
				{
					PUSH_BACK_ARG_STR(r, "-p", common->proto_path);
				}
			}
			if (common->max_buffer_size != 4096)
			{
				PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
			}
		}
		return r;
	};
};

/**
 * repeators = 
[
	{
		"in": "ipc:///tmp/control.pkt2",
		"bind" false,
		"outs": [
			"tcp://0.0.0.0:50000",
		]
	}
];
*/
class CfgRepeator
{
public:
	std::string url_in;
	bool bind;
	std::vector<std::string> url_outs;
	CfgRepeator() : url_in(""), bind(false) {};
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		if (url_in != "ipc:///tmp/control.pkt2")
		{
			PUSH_BACK_ARG_STR(r, "-i", url_in);
		}
		if (bind)
		{
			PUSH_BACK_ARG_LIT(r, "-s");
		}
		if (!((url_outs.size() == 1) && (url_outs[0] == "tcp://0.0.0.0:50000")))
		{
			for (int i = 0; i < url_outs.size(); i++)
			{
				PUSH_BACK_ARG_STR(r, "-o", url_outs[i]);
			}
		}
		
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->max_buffer_size != 4096)
			{
				PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
			}
		}
		return r;
	};
};

/**
 * script -c -i -o
 * scripts = 
[
	{
		"command: "",
		"in": "ipc:///tmp/control.pkt2",
		"outs": [
			"tcp://0.0.0.0:50000"
		]
	}
];
*/
class CfgScript
{
public:
	std::string url_in;
	std::string command;
	std::vector<std::string> url_outs;
	CfgScript() : url_in(""), command("") {};
	std::vector<std::string> args(CfgCommon *common)
	{
		std::vector<std::string> r;
		if (!command.empty())
		{
			PUSH_BACK_ARG_STR(r, "-c", command);
		}
		if (url_in != "ipc:///tmp/control.pkt2")
		{
			PUSH_BACK_ARG_STR(r, "-i", url_in);
		}
		if (!((url_outs.size() == 1) && (url_outs[0] == "tcp://0.0.0.0:50000")))
		{
			for (int i = 0; i < url_outs.size(); i++)
			{
				PUSH_BACK_ARG_STR(r, "-o", url_outs[i]);
			}
		}
		
		if (common)
		{
			if (common->verbosity > 0)
			{
				PUSH_BACK_ARG_LIT(r, "-" + std::string(common->verbosity, 'v'));
			}
			if (common->max_buffer_size != 4096)
			{
				PUSH_BACK_ARG_NUM(r, "-b", common->max_buffer_size);
			}
		}
		return r;
	};
};

class ProcessDescriptors
{
private:
	duk_context *context;
	std::string path;
public:
	CfgCommon cfgCommon;
	std::vector<CfgListenerTcp> cfgListenerTcp;
	std::vector<CfgListenerMqtt> cfgListenerMqtt;
	std::vector<CfgListenerFile> cfgListenerFile;
	std::vector<CfgPacket2Message> cfgPacket2Message;
	std::vector<CfgWriteFile> cfgWriteFile;
	std::vector<CfgWriteLmdb> cfgWriteLmdb;
	std::vector<CfgWritePq> cfgWritePq;
	std::vector<CfgWriteFcm> cfgWriteFcm;
	std::vector<CfgWriteGoogleSheets> cfgWriteGoogleSheets;
	std::vector<CfgRepeator> cfgRepeators;
	std::vector<CfgScript> CfgScripts;
	
	std::vector<ProcessDescriptor> descriptors;
	
	ProcessDescriptors()
	{
		context = duk_create_heap(NULL, NULL, NULL, this, duk_fatal_handler_process_descriptors);
	}

	void load_config() 
	{
		duk_push_global_object(context);
		
		duk_get_prop_string(context, -1, "proto_path");
		if (duk_is_string(context, -1)) 
			cfgCommon.proto_path = duk_get_string(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "bus_in");
		if (duk_is_string(context, -1)) 
			cfgCommon.bus_in = duk_get_string(context, -1);
		duk_pop(context); 

		duk_get_prop_string(context, -1, "bus_out");
		if (duk_is_string(context, -1)) 
			cfgCommon.bus_out = duk_get_string(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "max_file_descriptors");
		if (duk_is_number(context, -1)) 
			cfgCommon.max_file_descriptors = duk_get_number(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "max_buffer_size");
		if (duk_is_number(context, -1)) 
			cfgCommon.max_buffer_size = duk_get_number(context, -1);
		duk_pop(context);

		duk_get_prop_string(context, -1, "verbosity");
		if (duk_is_number(context, -1)) 
			cfgCommon.verbosity = duk_get_number(context, -1);
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

					if (duk_get_prop_string(context, -1, "compression_type"))
						cfg.compression_type = duk_get_number(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "escape_code"))
						cfg.escape_code = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "compression_offset"))
						cfg.compression_offset = duk_get_number(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "frequence_file"))
						cfg.frequence_file = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "codemap_file"))
						cfg.codemap_file = duk_get_string(context, -1);
					duk_pop(context);
					

					duk_get_prop_string(context, -1, "valid_sizes");
					if (duk_is_array(context, -1)) 
					{
						duk_size_t sz = duk_get_length(context, -1);
						for (duk_size_t v = 0; v < sz; v++) 
						{
							if (duk_get_prop_index(context, -1, v)) 
							{
								cfg.valid_sizes.push_back(duk_get_number(context, -1));
							}
							duk_pop(context);
						}
					}
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
					if (duk_get_prop_string(context, -1, "topic"))
						cfg.topic= duk_get_string(context, -1);
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

		// File listener
		duk_get_prop_string(context, -1, "file_listeners");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgListenerFile cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "file"))
						cfg.file = duk_get_string(context, -1);
					duk_pop(context);
					if (duk_get_prop_string(context, -1, "mode"))
						cfg.mode = duk_get_int(context, -1);
					duk_pop(context);
				} 
				duk_pop(context);
				cfgListenerFile.push_back(cfg);
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

		// write FCM
		duk_get_prop_string(context, -1, "write_fcm");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgWriteFcm cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					// FireBase token
					if (duk_get_prop_string(context, -1, "key"))
						cfg.key = duk_get_string(context, -1);
					duk_pop(context);
					// IMEI
					if (duk_get_prop_string(context, -1, "imei"))
						cfg.imei = duk_get_string(context, -1);
					duk_pop(context);
					
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
				cfgWriteFcm.push_back(cfg);
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
		
		// write repeators
		duk_get_prop_string(context, -1, "repeators");
		if (duk_is_array(context, -1)) 
		{
			duk_size_t n = duk_get_length(context, -1);
			for (duk_size_t i = 0; i < n; i++) 
			{
				CfgRepeator cfg;
				if (duk_get_prop_index(context, -1, i)) 
				{
					if (duk_get_prop_string(context, -1, "in"))
						cfg.url_in = duk_get_string(context, -1);
					duk_pop(context);

					if (duk_get_prop_string(context, -1, "bind"))
						cfg.bind = duk_get_boolean(context, -1);
					duk_pop(context);

					duk_get_prop_string(context, -1, "outs");
					if (duk_is_array(context, -1)) 
					{
						duk_size_t sn = duk_get_length(context, -1);
						for (duk_size_t s = 0; s < sn; s++) 
						{
							if (duk_get_prop_index(context, -1, s)) 
								cfg.url_outs.push_back(duk_get_string(context, -1));
							duk_pop(context);
						}
					}
					duk_pop(context);
				} 
				duk_pop(context);
				cfgRepeators.push_back(cfg);
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
		const std::string &a_path,
		const std::string &config_file_name
	)
	{
		path = a_path;

		std::string v = file2string(config_file_name);
		duk_eval_string(context, v.c_str());
		duk_pop(context);  // ignore result
		
		cfgListenerTcp.clear();
		cfgListenerMqtt.clear();
		cfgListenerFile.clear();
		cfgPacket2Message.clear();
		cfgWriteFile.clear();
		cfgWriteLmdb.clear();
		cfgWritePq.clear();
		cfgWriteFcm.clear();
		cfgWriteGoogleSheets.clear();
		cfgRepeators.clear();
		CfgScripts.clear();
		
		load_config();
		return 0;
	}
	
	int init()
	{
		descriptors.clear();
		
		for (std::vector<CfgListenerTcp>::iterator it(cfgListenerTcp.begin()); it != cfgListenerTcp.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "tcpreceiver", it->args(&cfgCommon)));
		}
		
		for (std::vector<CfgListenerMqtt>::iterator it(cfgListenerMqtt.begin()); it != cfgListenerMqtt.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "mqtt-receiver", it->args(&cfgCommon)));
		}

		for (std::vector<CfgListenerFile>::iterator it(cfgListenerFile.begin()); it != cfgListenerFile.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "freceiver", it->args(&cfgCommon)));
		}

		for (std::vector<CfgPacket2Message>::iterator it(cfgPacket2Message.begin()); it != cfgPacket2Message.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "pkt2receiver", it->args(&cfgCommon)));
		}
		
		for (std::vector<CfgWriteFile>::iterator it(cfgWriteFile.begin()); it != cfgWriteFile.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "handlerline", it->args(&cfgCommon)));
		}
		
		for (std::vector<CfgWriteLmdb>::iterator it(cfgWriteLmdb.begin()); it != cfgWriteLmdb.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "handlerlmdb", it->args(&cfgCommon)));
		}
		
		for (std::vector<CfgWritePq>::iterator it(cfgWritePq.begin()); it != cfgWritePq.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "handlerpq", it->args(&cfgCommon)));
		}

		for (std::vector<CfgWriteFcm>::iterator it(cfgWriteFcm.begin()); it != cfgWriteFcm.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "handlerfcm", it->args(&cfgCommon)));
		}

		for (std::vector<CfgWriteGoogleSheets>::iterator it(cfgWriteGoogleSheets.begin()); it != cfgWriteGoogleSheets.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "handler-google-sheets", it->args(&cfgCommon)));
		}

		for (std::vector<CfgRepeator>::iterator it(cfgRepeators.begin()); it != cfgRepeators.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "repeator", it->args(&cfgCommon)));
		}

		for (std::vector<CfgScript>::iterator it(CfgScripts.begin()); it != CfgScripts.end(); ++it)
		{
			descriptors.push_back(ProcessDescriptor(path, "script", it->args(&cfgCommon)));
		}

		return 0;
	}

	int start()
	{
		for (int i = 0; i < descriptors.size(); i++)
		{
			std::stringstream ss;
			for (int j = 0; j < descriptors[i].args.size(); j++)
			{
				ss << " " << descriptors[i].args[j];
			}
			LOG(INFO) << descriptors[i].cmd << ss.str();
			descriptors[i].start();
		}
		return 0;
	}
	
	int check()
	{
		for (int i = 0; i < descriptors.size(); i++)
		{
			if (!descriptors[i].is_running())
			{
				time_t last_started = descriptors[i].last_start;
				time_t now = time(NULL);
				LOG(ERROR) << "Re-start " << descriptors[i].cmd << " elapsed time approximately " << now - last_started << "s. Count: " << descriptors[i].start_count + 1;
				descriptors[i].start();
			}
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
	if (processes.load(config->path, config->file_name) != ERR_OK)
		return ERRCODE_CONFIG;
	processes.init();
	processes.start();
	config->stop_request = 0;
	while (!config->stop_request)
	{
		processes.check();
		SLEEP(1);
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
