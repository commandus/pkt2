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
#include "write2lmdb-config.h"
#include "lmdbwriter.h"

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
	google::InitGoogleLogging(argv[0]);
    google::SetLogDestination(google::INFO, PROGRAM_NAME);

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

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, runner, stopNWait, done);
	}
	else
	{
		LOG(INFO) << "Start..";
		runner();
		done();
	}

	return reslt;
}
