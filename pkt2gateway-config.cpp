#include "pkt2gateway-config.h"
#include <limits.h>
#include <stdlib.h>
#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include "platform.h"
#include "utilfile.h"
#else
#endif

#include "argtable3/argtable3.h"

#include "errorcodes.h"

#define DEF_PROTO_PATH				            "proto"
#define DEF_MODE                                "raw"
#define DEF_QUEUE_OUT                           "ipc:///tmp/message.pkt2"
#define DEF_BUFFER_SIZE                         4096

Config::Config
(
    int argc,
    char* argv[]
)
{
    stop_request = false;
	lastError = parseCmd(argc, argv);
	buffer_size = DEF_BUFFER_SIZE;
}

int Config::error()
{
    return lastError;
}

/**
 * @brief Parse command line into struct ClientConfig
 * @return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int Config::parseCmd
(
        int argc,
        char* argv[]
)
{
        struct arg_str *a_message_out_url = arg_str0("o", "output", "<queue url>", "Default " DEF_QUEUE_OUT);
        struct arg_str *a_packet = arg_str0(NULL, NULL, "<data>", "hex, JSON or raw data. Or eread stdin");
        struct arg_str *a_mode = arg_str0("m", "mode", "raw|hex|<packet.message>", "Data format. Default raw.");
        struct arg_file *a_input_file = arg_file0("f", "file", "<file name>", "otherwise stdin");
        struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
        struct arg_str *a_force_message = arg_str0(NULL, "message", "<packet.message>", "force message");

        struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Repeat each message. Default 1.");
        struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on repeats in seconds. Default 0");

    	struct arg_str *a_table_alias = arg_strn("T", "table-alias", "<alias=message>", 0, 100, "set table alias for message");
	    struct arg_str *a_field_alias = arg_strn("F", "field-alias", "<alias=field>", 0, 100, "set field alias");
        struct arg_str *a_properties = arg_strn("P", "properties", "<property=name>", 0, 100, "set property");

        struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

        struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
        struct arg_end *a_end = arg_end(20);

        void* argtable[] = {
        		a_proto_path, a_force_message,
        		a_packet, a_mode, a_input_file, a_message_out_url,
				a_retries, a_retry_delay,
                a_table_alias, a_field_alias, a_properties,
                a_verbosity,
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

        if (a_proto_path->count)
        	proto_path = *a_proto_path->sval;
        else
        	proto_path = DEF_PROTO_PATH;
		
		// get real path
		char b[PATH_MAX];
		char *pp = realpath(proto_path.c_str(), b);
        if (pp)
            proto_path = std::string(pp);
        else {
            std::cerr << ERR_INVALID_PROTO_PATH << std::endl;
            arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
            return ERRCODE_INVALID_PROTO_PATH;
        }

        if (a_force_message->count)
        	force_message = *a_force_message->sval;
        else
        	force_message = "";

        if (a_mode->count)
        	mode = *a_mode->sval;
        else
        	mode = DEF_MODE;

        if (a_packet->count)
        	packet = *a_packet->sval;
        else
        	packet = "";

        if (a_input_file->count)
            file_name = *a_input_file->filename;
        else
        	file_name = "";	// stdin

        if (a_message_out_url->count)
        	message_out_url = *a_message_out_url->sval;
        else
        	message_out_url = DEF_QUEUE_OUT;

        verbosity = a_verbosity->count;

        if (a_retries->count)
        	retries = *a_retries->ival;
        else
        	retries = 1;

        if (a_retry_delay->count)
			retry_delay = *a_retry_delay->ival;
        else
			retry_delay = 0;

        for (int i = 0; i < a_table_alias->count; i++) {
            std::string line = a_table_alias->sval[i];
            size_t p = line.find('=');
            if (p == std::string::npos)
                continue;
            std::string n = line.substr(0, p);
            std::string v = line.substr(p + 1);
            tableAliases[n] = v;
        }

        for (int i = 0; i < a_field_alias->count; i++) {
            std::string line = a_field_alias->sval[i];
            size_t p = line.find('=');
            if (p == std::string::npos)
                continue;
            std::string n = line.substr(0, p);
            std::string v = line.substr(p + 1);
            fieldAliases[n] = v;
        }

        for (int i = 0; i < a_properties->count; i++) {
            std::string line = a_properties->sval[i];
            size_t p = line.find('=');
            if (p == std::string::npos)
                continue;
            std::string n = line.substr(0, p);
            std::string v = line.substr(p + 1);
            properties[n] = v;
        }

        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
}
