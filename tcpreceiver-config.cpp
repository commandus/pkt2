#include "tcpreceiver-config.h"
#include <unistd.h>
#include <limits.h>
#include <argtable3.h>

#define DEF_PORT                 50052
#define DEF_ADDRESS              "0.0.0.0"
#define DEF_QUEUE                "ipc:///tmp/packet.pkt2"
#define DEF_BUFFER_SIZE          4096

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
	struct arg_str *a_interface = arg_str0("i", "ipaddr", "<IP address>", "Network interface name or address. Default 0.0.0.0");
	struct arg_int *a_port = arg_int0("l", "listen", "<port>", "TCP port to listen. Default 50052");

	struct arg_str *a_message_url = arg_str0("o", "output", "<bus url>", "Default ipc:///tmp/packet.pkt2");
	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Default 4096 bytes");
	struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	
	// decompression
	struct arg_int *a_compression_type = arg_int0("c", "compression", "<number>", "1- Huffman(modified). Default 0- none.");
	struct arg_str *a_escape_code = arg_str0("e", "escapecode", "<BINARY CODE>", "Escape binary code is a prefix for 8 bit value(not a Huffman code). Default none.");
	struct arg_str *a_eof_code = arg_str0("E", "eofcode", "<BINARY CODE>", "EOF code. Default none (use varint length descriptor).");
	struct arg_int *a_compression_offset = arg_int0("s", "start", "<offset>", "Valid with -c option. Default 0."); ///< offset where data compression is started
	struct arg_file *a_frequence_file = arg_file0("f", "freq", "<file>", "Compression frequence file name (in conjuction with -m) .");
	struct arg_file *a_codemap_file = arg_file0("m", "map", "<file>", "Compression code dictionary file name.");
	/// empty- do not control size of decompressed pacxet. If size if not in the list, try get packet as-is (w/o decompresson).
	struct arg_int *a_valid_sizes = arg_intn("p", "packetsize", "<bytes number>", 0, 256, "Default any sizes."); 
	
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_int *a_max_fd = arg_int0(NULL, "maxfd", "<number>", "Set max file descriptors. 0- use default (1024).");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { 
			a_interface, a_port, a_message_url, a_buffer_size, 
			a_retries, a_retry_delay,
			a_compression_type, a_escape_code, a_eof_code, a_compression_offset, a_frequence_file, a_codemap_file, 
			a_valid_sizes,
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

	if (a_interface->count)
		intface = *a_interface->sval;
	else
		intface = DEF_ADDRESS;

	if (a_port->count)
		port = *a_port->ival;
	else
		port = DEF_PORT;

	if (a_message_url->count)
		message_url = *a_message_url->sval;
	else
		message_url = DEF_QUEUE;

	if (a_buffer_size->count)
		buffer_size = *a_buffer_size->ival;
	else
		buffer_size = DEF_BUFFER_SIZE;

	if (a_retries->count)
		retries = *a_retries->ival;
	else
		retries = 0;

	if (a_retry_delay->count)
		retry_delay = *a_retry_delay->ival;
	else
		retry_delay = 60;

	// decompression options
	if (a_compression_type->count)
		compression_type = *a_compression_type->ival;
	else
		compression_type = 0;

	if (a_escape_code->count)
		escape_code = *a_escape_code->sval;
	else
		escape_code = "";

	if (a_eof_code->count)
		eof_code = *a_eof_code->sval;
	else
		eof_code = "";

	if (a_compression_offset->count)
		compression_offset = *a_compression_offset->ival;
	else
		compression_offset = 0;

	if (a_frequence_file->count)
		frequence_file = std::string(*a_frequence_file->filename);
	else
		frequence_file = "";

	if (a_codemap_file->count)
		codemap_file = std::string(*a_codemap_file->filename);
	else
		codemap_file = "";

	// empty- do not control size of decompressed pacxet. If size if not in the list, try get packet as-is (w/o decompresson).
	for (int i = 0; i < a_valid_sizes->count; i++)
	{
		valid_sizes.push_back(a_valid_sizes->ival[i]);
	}

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
