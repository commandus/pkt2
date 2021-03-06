/**
 * Read packet from nanomsg socket, parse and emit
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include <glog/logging.h>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#endif

#include "platform.h"
#include "daemonize.h"
#include "message2gateway-config.h"

#include "errorcodes.h"

Config *config;

/**
  * @return:  0- success
  * @see errorcodes.h
  */
int run
(
	Config *config
);

/**
  * Return 0- success
  */
int stop
(
	Config *config
);

int reload
(
	Config *config
);

void stopNWait()
{
	std::cerr << MSG_STOP;
	stop(config);
}

void done()
{
	if (config)
	{
		if (config->verbosity >= 2)
			std::cerr << MSG_DONE;
	}
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
		std::cerr << MSG_SIGNAL;
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
		exit(ERRCODE_NO_CONFIG);
    if (config->error() != 0)
	{
		LOG(ERROR) << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
	}

    INIT_LOGGING(PROGRAM_NAME)

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, config->path, runner, stopNWait, done);
	}
	else
	{
		LOG(INFO) << MSG_START;
		runner();
		done();
	}

	return reslt;
}
