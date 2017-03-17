#include <stdio.h>
#include <signal.h>

#include <iostream>
#include <glog/logging.h>
#include <stdlib.h>

#include "platform.h"
#include "daemonize.h"
#include "tcpemitter-config.h"
#include "tcpemitterline.h"
#include "errorcodes.h"

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
		Daemonize daemonize(PROGRAM_NAME, run, stopNWait, done);
	}
	else
	{
		LOG(INFO) << MSG_START;
		run();
		done();
	}

	return reslt;
}
