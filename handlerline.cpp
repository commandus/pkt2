/**
 *   Read protobuf messages from nanomsg socket ipc:///tmp/output.pkt2 (-q)
 *   Print to stdout
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
#include "handlerline-config.h"
#include "linewriter.h"

#include "errorcodes.h"

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
 * 
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

	config = new Config(argc, argv);
	if (!config)
		exit(3);
    if (config->error() != 0)
	{
		LOG(ERROR) << ERR_COMMAND;
		exit(config->error());
	}

    INIT_LOGGING(PROGRAM_NAME)
    
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
