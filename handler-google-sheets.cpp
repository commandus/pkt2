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

	int r = loadGoogleToken(
		config->service_account,
		config->subject_email, 
		file2string(config->pemkeyfilename),
		config->scope, 
		config->audience,
		config->expires,
		config->token
    );
    
	if (r || config->token.empty())
	{
		LOG(ERROR) << ERR_TOKEN_BEARER << " " << config->pemkeyfilename;
		exit(ERRCODE_TOKEN_BEARER);
	}

	if (config->verbosity >= 2)
		LOG(INFO) << "Token bearer: " << config->token;

	config->sheet = "1iDg77CjmqyxWyuFXZHat946NeAEtnhL6ipKtTZzF-mo";
	
	GoogleSheets gs(config->sheet, config->token);
	std::string range = "A1:A2"; // Класс!
	ValueRange cells;
	if (gs.getRange(range, cells))
	{
		LOG(ERROR) << ERR_GS_RANGE;
		exit(ERRCODE_GS_RANGE);
	}
	else
		LOG(ERROR) << cells.toString();

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
