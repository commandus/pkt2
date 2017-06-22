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
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST;
		reload(config);
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
	setSignalHandler(SIGHUP);
    reslt = 0;

	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_CONFIG);
    if (config->error() != 0)
	{
		LOG(ERROR) << getErrorDescription(config->error());
		exit(config->error());
	}

    INIT_LOGGING(PROGRAM_NAME)

	if (config->verbosity >= 2)
	{
		LOG(INFO) << "arguments: " << arg2String(argc, argv);
		LOG(INFO) << "speadsheet: " << config->sheet;
		LOG(INFO) << "subject email: " << config->subject_email;
	}

// #define TEST
#ifdef TEST	
    ValueRange cells;
	if (!config->google_sheets->get("Sheet1!A1:A2", cells))
	{
		LOG(ERROR) << ERR_GS_RANGE;
		exit(ERRCODE_GS_RANGE);
	}
	else
		LOG(ERROR) << cells.toString();
	/*
	std::ifstream myfile("1.csv");
	if (myfile.is_open())
	{
		ValueRange newcells("Class!K11:N15", myfile);
		if (!config->google_sheets->put(newcells))
		{
			LOG(ERROR) << ERR_GS_RANGE;
			exit(ERRCODE_GS_RANGE);
		}
		myfile.close();
	}
	*/
	
	std::ifstream myfile2("1.csv");
	if (myfile2.is_open())
	{
		ValueRange newcells("Class!A1:D1", myfile2);
		if (!config->google_sheets->append(newcells))
		{
			LOG(ERROR) << ERR_GS_RANGE;
			exit(ERRCODE_GS_RANGE);
		}
		
		myfile2.close();
	}
#endif

	if (config->verbosity >= 2)
		LOG(INFO) << "Token bearer: " << config->token;
    
	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, config->path, runner, stopNWait, done, 0);
	}
	else
	{
		LOG(INFO) << MSG_START;
		runner();
		done();
	}

	return reslt;
}
