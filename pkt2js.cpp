#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>

#ifdef ENABLE_LOG
#include <glog/logging.h>
#endif

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
#ifdef ENABLE_LOG
		LOG(ERROR) << ERR_NO_CONFIG;
#endif
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
#if defined(_WIN32) || defined(_WIN64)
#else
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST << std::endl;
		reload(config);
		break;
#endif
	default:
		std::cerr << MSG_SIGNAL << signal << std::endl;
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
	// TODO
    // Signal handler
	// terminate called after throwing an instance of 'std::length_error'
	// what():  basic_string::_M_create
	// *** Aborted at 1582687385 (unix time) try "date -d @1582687385" if you are using GNU date ***
	// PC: @     0x7f77a1ff6428 gsignal
	// *** SIGABRT (@0x3e80000183c) received by PID 6204 (TID 0x7f77a3521740) from PID 6204; stack trace: ***
    // setSignalHandler(SIGINT);
#if defined(_WIN32) || defined(_WIN64)
#else
	setSignalHandler(SIGHUP);
#endif

    reslt = 0;

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_PARSE_COMMAND);

#ifdef ENABLE_LOG
	INIT_LOGGING(PROGRAM_NAME)
#endif

    if (config->error() != 0)
	{
#ifdef ENABLE_LOG
		if (config->error() != ERRCODE_HELP_REQUESTED)
			LOG(ERROR) << ERR_COMMAND;
#endif
		exit(ERRCODE_COMMAND);
	}
	if (config->daemonize)
	{
#ifdef ENABLE_LOG
		LOG(INFO) << MSG_DAEMONIZE;
#endif
		Daemonize daemonize(PROGRAM_NAME, config->path, run, stopNWait, done, config->max_fd);
	}
	else
	{
#ifdef ENABLE_LOG
		LOG(INFO) << MSG_START;
#endif
		if (config->max_fd > 0)
			Daemonize::setFdLimit(config->max_fd);
		run();
		done();
	}

	return reslt;
}
