#include <stdio.h>
#include <signal.h>

#include <iostream>
#include <glog/logging.h>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#endif
#include <stdlib.h>

#include "platform.h"
#include "daemonize.h"
#include "messageemitter-config.h"
#include "messageemitterline.h"
#include "errorcodes.h"

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
		LOG(ERROR) << ERR_PARSE_COMMAND;
		return;
	}
	int n = 0;
	while (!config->stop_request)
	{
		reslt = tcp_emitter_line(config);
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
	{
		LOG(ERROR) << ERR_PARSE_COMMAND;
		exit(ERRCODE_PARSE_COMMAND);
	}
    if (config->error() != 0)
	{
		LOG(ERROR) << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
	}

    INIT_LOGGING(PROGRAM_NAME)

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, config->path, run, stopNWait, done);
	}
	else
	{
		LOG(INFO) << MSG_START;
		run();
		done();
	}

	return reslt;
}
