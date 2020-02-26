#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include <glog/logging.h>

#include "platform.h"
#include "daemonize.h"
#include "errorcodes.h"

#include "pkt2js-config.h"
#include "pkt2js-impl.h"

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
		reslt = pkt2js(config);
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
		std::cerr << MSG_INTERRUPTED << std::endl;
		stopNWait();
		done();
		break;
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST << std::endl;
		reload(config);
		break;
	default:
		std::cerr << MSG_SIGNAL << signal << std::endl;
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
	// TODO
    // Signal handler
	// terminate called after throwing an instance of 'std::length_error'
	// what():  basic_string::_M_create
	// *** Aborted at 1582687385 (unix time) try "date -d @1582687385" if you are using GNU date ***
	// PC: @     0x7f77a1ff6428 gsignal
	// *** SIGABRT (@0x3e80000183c) received by PID 6204 (TID 0x7f77a3521740) from PID 6204; stack trace: ***
    // setSignalHandler(SIGINT);
	setSignalHandler(SIGHUP);

    reslt = 0;

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_PARSE_COMMAND);

    INIT_LOGGING(PROGRAM_NAME)

    if (config->error() != 0)
	{
		if (config->error() != ERRCODE_HELP_REQUESTED)
			LOG(ERROR) << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
	}
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
