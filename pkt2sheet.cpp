/**
 *   Read protobuf messages from stdin
 *   Print to Google Sheets
 *
 *   Usage (default values):
 *   	pkt2sheet
 *
 *   Options
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
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#endif

#include "platform.h"
#include "daemonize.h"
#include "pkt2sheet-config.h"
#include "google-sheets-writer-from-file.h"

#include "utilstring.h"

#include "errorcodes.h"
#include "google-sheets.h"

Config *config;

void stopNWait()
{
	if (config)
		stop(config);
}

void done()
{
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
		SLEEP(config->retry_delay);
		n++;
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
#if defined(_WIN32) || defined(_WIN64)
#else
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST;
		reload(config);
		break;
#endif
	default:
		std::cerr << MSG_SIGNAL << signal;
	}
}

#if defined(_WIN32) || defined(_WIN64)
#else
void setSignalHandler(int signal)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	sigaction(signal, &action, NULL);
}
#endif

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
#if defined(_WIN32) || defined(_WIN64)
#else
    setSignalHandler(SIGINT);
	setSignalHandler(SIGHUP);
#endif
    reslt = 0;

	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_CONFIG);

	INIT_LOGGING(PROGRAM_NAME)

    if (config->error() != 0)
	{
		if (config->error() != ERRCODE_HELP_REQUESTED)
			LOG(ERROR) << getErrorDescription(config->error());
		exit(config->error());
	}

	if (config->verbosity >= 2)
	{
		LOG(INFO) << "arguments: " << pkt2utilstring::arg2String(argc, argv);
		LOG(INFO) << "speadsheet: " << config->sheet;
		LOG(INFO) << "subject email: " << config->subject_email;
	}

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
