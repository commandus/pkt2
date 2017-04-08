/**
 *   Read protobuf messages from nanomsg socket ipc:///tmp/output.pkt2 (-q)
 *   Print to Google Sheets
 *
 *   Usage (default values):
 *   	handlerline -q ipc:///tmp/packet.pkt2 -m 0
 *
 *   Options
 *   	-m 0- JSON
 *
 *   Error codes:
 *           0- success
 *
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <glog/logging.h>

#include "platform.h"
#include "daemonize.h"
#include "handler-google-sheets-config.h"
#include "google-sheets-writer.h"

#include "utilstring.h"

#include "errorcodes.h"
#include "google-sheets.h"

Config *config;

void stopNWait()
{
    LOG(INFO) << MSG_STOP;
	if (config)
		stop(config);
}

void done()
{
    LOG(INFO) << MSG_DONE;
}

int reslt;

void runner()
{
	if (!config)
	{
		LOG(ERROR) << ERR_NO_CONFIG;
		return;
	}
	int n = 0;
	while (!config->stop_request)
	{
		reslt = run(config);
		if (n >= config->retries)
			break;
		n++;
		sleep(config->retry_delay);
	}
}

void signalHandler(int signal)
{
        switch(signal)
        {
        case SIGINT:
        		std::cerr << MSG_INTERRUPTED;
                stopNWait();
                done();
                break;
        default:
        		std::cerr << MSG_SIGNAL << signal;
        }
}

void setSignalHandler(int signal)
{
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_handler = &signalHandler;
        sigaction(signal, &action, NULL);
}

/**
 * @returns @see errorcodes.h
 */
int main
(
    int argc, 
    char *argv[]
)
{
    // Signal handler
    setSignalHandler(SIGINT);
    reslt = 0;

	config = new Config(argc, argv);
	if (!config)
		exit(3);
    if (config->error() != 0)
	{
		LOG(ERROR) << ERR_COMMAND;
		exit(config->error());
	}

    INIT_LOGGING(PROGRAM_NAME)

	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);

	std::string json_google_service = file2string(config->json);
	int r;
	
	std::string s = config->service_account;
	readGoogleTokenJSON(config->json, s, config->pemkey);
	if (config->pemkey.empty())
		config->pemkey = file2string(config->pemkeyfilename);
	else
		config->service_account = s;
	
	if (config->verbosity >= 2)
	{
		LOG(INFO) << "subject email: " << config->subject_email;
	}

	GoogleSheets gs
	(
		config->sheet, 
		config->service_account,
		config->subject_email, 
		config->pemkey,
		config->scope, 
		config->audience
	);

	ValueRange cells;
	if (gs.getRange("A1:A2", cells))
	{
		LOG(ERROR) << ERR_GS_RANGE;
		exit(ERRCODE_GS_RANGE);
	}
	else
		LOG(ERROR) << cells.toString();
	
	std::ifstream myfile("1.csv");
	if (myfile.is_open())
	{
		ValueRange newcells("Class!K11:N15", myfile);
LOG(ERROR) << "Values: " << newcells.toJSON();;
		gs.putRange(newcells);
		myfile.close();
	}

	if (config->verbosity >= 2)
		LOG(INFO) << "Token bearer: " << config->token;
    
	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, runner, stopNWait, done, config->max_fd);
	}
	else
	{
		LOG(INFO) << MSG_START;
		if (config->max_fd > 0)
			Daemonize::setFdLimit(config->max_fd);
		runner();
		done();
	}

	return reslt;
}
