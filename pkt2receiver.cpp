#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include <glog/logging.h>

#include "platform.h"
#include "daemonize.h"
#include "errorcodes.h"

#include "pkt2receiver-config.h"
#include "pkt2receivernano.h"

Config *config;

void stopNWait()
{
	stop(config);
}

void done()
{
}

int reslt;

void run()
{
	if (!config)
	{
		LOG(ERROR) << ERR_NO_CONFIG;
		return;
	}
	int n = 0;
	while (!config->stop_request)
	{
		reslt = pkt2_receiever_nano(config);
		if (n >= config->retries)
			break;
		sleep(config->retry_delay);
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

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_PARSE_COMMAND);
    if (config->error() != 0)
	{
		LOG(ERROR) << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
	}

    INIT_LOGGING(PROGRAM_NAME)

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, config->path, run, stopNWait, done, config->max_fd);
	}
	else
	{
		LOG(INFO) << MSG_START;
		if (config->max_fd > 0)
			Daemonize::setFdLimit(config->max_fd);
		run();
		done();
	}

	return reslt;
}
