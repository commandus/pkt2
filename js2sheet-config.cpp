#include "js2sheet-config.h"
#include <iostream>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include "argtable3/argtable3.h"
#include "utilstring.h"
#include "errorcodes.h"

#define DEF_BUFFER_SIZE				4096
#define DEF_PROTO_PATH				"proto"

// Google service
#define DEF_GOOGLE_JSON				"cert/pkt2-sheets.json"
#define DEF_TOKEN_FILE				".token-bearer.txt"

#define DEF_SCOPE					"https://www.googleapis.com/auth/spreadsheets"
#define DEF_AUDIENCE				"https://www.googleapis.com/oauth2/v4/token"
#define DEF_EXPIRES					3600

void ontokenbearer
(
    void *env,
	const std::string &value,
	int status
)
{
	Config *config = (Config *) env;
	if (status == 0)
	{
		pkt2utilstring::string2file(config->token_file, value);
		std::cerr << "New token bearer: " << value << " saved in " << config->token_file << std::endl;
	}
	else
		std::cerr << "Error " << status << " getting token bearer: " << value << std::endl;;
}

Config::Config
(
    int argc, 
    char* argv[]
)
{
	stop_request = 0;
	lastError = parseCmd(argc, argv);

	if (lastError != 0)
	{
		google_sheets = NULL;
		return;
	}
	
	std::string json_google_service = pkt2utilstring::file2string(json);

	std::string pub_email;
	readGoogleTokenJSON(json_google_service, service_account, pemkey, pub_email);
	if (pemkey.empty() || service_account.empty())
	{
 		lastError = ERRCODE_GS_INVALID_CFG_JSON;
		google_sheets = NULL;
		return;
	}
	
	if (subject_email.empty())
		subject_email = pub_email;

	std::string last_token = pkt2utilstring::file2string(token_file);
	google_sheets = new GoogleSheets(
		spreadsheet,
		last_token,
		service_account,
		subject_email,
		pemkey,
		scope,
		audience,
		&ontokenbearer,
		this
	);
}

Config::~Config()
{
	if (google_sheets)
		delete google_sheets;
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
	struct arg_str *a_proto_path = arg_str0("p", "protos", "<path>", "proto file directory. Default " DEF_PROTO_PATH);
	struct arg_str *a_filename_input = arg_str0("i", "input", "<file name>", "Default stdin");
  struct arg_int *a_retries = arg_int0("r", "repeat", "<n>", "Restart listen. Default 0.");
	struct arg_int *a_retry_delay = arg_int0("y", "delay", "<seconds>", "Delay on restart in seconds. Default 60.");
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "Start as daemon/service");
	struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 3, "Verbosity level. 3- debug");

	// Google service auth
	// JSON contains all others
	struct arg_file *a_json = arg_file0("g", "google", "<file>", "JSON service file name. Default " DEF_GOOGLE_JSON);
	struct arg_file *a_token_file = arg_file0("B", "bearer", "<file>", "save token bearer. Default " DEF_TOKEN_FILE);
	struct arg_str *a_subject_email = arg_str1("e", "sub", "<email>", "subject email.");
	struct arg_str *a_spreadsheet = arg_str1("s", "spreadsheet", "<id>", "Google spreadsheet id");
	struct arg_str *a_sheet = arg_str1("t", "sheet", "<name>", "Sheet name");
	struct arg_int *a_format_number = arg_int0(NULL, "format", "<number>", "Default 0");

	struct arg_int *a_buffer_size = arg_int0("b", "buffer", "<size>", "Receiver buffer size. Default 4096");
	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = {
		a_proto_path, a_filename_input,
		a_retries, a_retry_delay,
		// OAuth, Google sheets
		a_json, a_token_file, a_subject_email, a_spreadsheet, a_sheet, a_format_number,
		a_daemonize, a_verbosity, a_buffer_size, a_help, a_end 
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
			if (a_help->count)
				return ERRCODE_HELP_REQUESTED;
			else
				return ERRCODE_PARSE_COMMAND;
	}

	
	if (a_filename_input->count)
		filename_input = *a_filename_input->sval;
	else
		filename_input = "";

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

	verbosity = a_verbosity->count;

	daemonize = a_daemonize->count > 0;

	if (a_format_number->count)
		format_number = *a_format_number->ival;
	else
		format_number = 0;

	// OAuth, Google sheets
	if (a_spreadsheet->count)
		spreadsheet = *a_spreadsheet->sval;

	if (a_sheet->count)
		sheet = *a_sheet->sval;

	if (a_json->count)
		json = *a_json->filename;
	else
		json = DEF_GOOGLE_JSON;

	if (a_token_file->count)
		token_file = *a_token_file->filename;
	else
		token_file = DEF_TOKEN_FILE;

	if (a_subject_email->count)
		subject_email = *a_subject_email->sval;

	scope = DEF_SCOPE;
	audience = DEF_AUDIENCE;
	expires = DEF_EXPIRES;

	char wd[PATH_MAX];
	path = getcwd(wd, PATH_MAX);	

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return ERR_OK;
}
