/**
 *   Read protobuf messages from nanomsg socket ipc:///tmp/output.pkt2 (-q)
 *   Store to LMDB database file (-p)
 *
 *   Usage (default values):
 *   	write2lmdb -q ipc:///tmp/packet.pkt2 -p db -f 0 -m 0664
 *   -f LMDB database file flags
 *   -m LMDB database file open mode flags
 *
 *   Error codes:
 *           0- success
 *
 *
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include <glog/logging.h>

#include "platform.h"
#include "daemonize.h"
#include "handlerlmdb-config.h"
#include "lmdbwriter.h"

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
		Daemonize daemonize(PROGRAM_NAME, config->path, runner, stopNWait, done, config->max_fd);
	}
	else
	{
		LOG(INFO) << "Start..";
		if (config->max_fd > 0)
			Daemonize::setFdLimit(config->max_fd);
		runner();
		done();
	}

	return reslt;
}
