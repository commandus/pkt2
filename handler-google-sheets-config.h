#ifndef HANDLER_LINE_CONFIG_H
#define HANDLER_LINE_CONFIG_H     1

#include <string>
#include <vector>
#include "google-sheets.h"

#define PROGRAM_NAME				"handler-google-sheets"
#define PROGRAM_DESCRIPTION			"Google sheet printer"

void ontokenbearer
(
	void *env,
	const std::string &value,
	int status
);

/**
  * @brief google-sheets-writer command line options
  * @see google-sheets-writer.cpp
  */
class Config
{
private:
	int lastError;
	/**
	* Parse command line
	* Return 0- success
	**/
	int parseCmd
	(
		int argc,
		char* argv[]
	);
public:
	Config(int argc, char* argv[]);
	virtual ~Config();
	int error();

	std::string message_url; 		///< nano message URL
	int retries;             		///< default 1
	int retry_delay;         		///<
	std::vector <std::string> allowed_messages;

	// proto
	std::string proto_path;			///< proto files directory path
	int mode;		        		///< default 0
	int buffer_size;        		///< default 2048

	// OAuth, Google sheets
	std::string client;
	std::string secret;
	std::string sheet;

	int format_number;				///< which format to use

	bool daemonize;
	int max_fd;						///< 0- use default max file descriptor count per process
	int stop_request;
	int accept_socket;

	int verbosity;			        ///< default 0

	// Google service
	std::string json;				///< JSON contains all others

	std::string service_account;	///< Client id. default "102274909249528994829"
	std::string subject_email;		///< default "andrei.i.ivanov@commandus.com"
	std::string pemkey;				///<
	std::string scope;				///< default "https://www.googleapis.com/auth/spreadsheets"
	std::string audience;			///< default "https://www.googleapis.com/oauth2/v4/token"
	int expires;					///< default 3600
	// Google sheet
	std::string spreadsheet;		///< Google spreadsheet id
	std::string sheet_id;			///< Sheet name
	// Google token
	std::string token;
	std::string token_file;			///< save token bearer in the file
	GoogleSheets *google_sheets;
};


#endif
