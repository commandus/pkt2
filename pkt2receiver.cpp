#include <stdio.h>
#include <signal.h>

//#include <iostream>
#include <glog/logging.h>

#include "platform.h"
#include "daemonize.h"

#include "pkt2receiver-config.h"
#include "pkt2receivernano.h"

Config *config;

void stopNWait()
{
    LOG(INFO) << "Stop..";
	if (config)
		stop(config);
}

void done()
{
    LOG(INFO) << "done";
}

int reslt;

void run()
{
	if (!config)
	{
		LOG(ERROR) << "config corrupted.";
		return;
	}
	int n = 0;
	while (!config->stop_request)
	{
		reslt = pkt2_receiever_nano(config);
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
                LOG(INFO) << "Interrupted";
                stopNWait();
                done();
                LOG(INFO) << "exit";
                break;
        default:
                LOG(INFO) << "Signal " << signal;
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
	google::InitGoogleLogging(argv[0]);
    google::SetLogDestination(google::INFO, PROGRAM_NAME);

    // Signal handler
    setSignalHandler(SIGINT);
    reslt = 0;

	config = new Config(argc, argv);
	if (!config)
		exit(5);
    if (config->error() != 0)
	{
		LOG(ERROR) << "exit, invalid command line options or help requested.";
		exit(config->error());
	}

	if (config->daemonize)
	{
		LOG(INFO) << "Start as daemon, use syslog";
		Daemonize daemonize(PROGRAM_NAME, run, stopNWait, done);
	}
	else
	{
		LOG(INFO) << "Start..";
		run();
		done();
	}

	return reslt;
}