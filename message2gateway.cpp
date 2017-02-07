#include <stdio.h>
#include <signal.h>

#include <glog/logging.h>

#include "platform.h"
#include "daemonize.h"
#include "message2gateway-config.h"

Config *config;

/**
  * Return:  0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open
  *          5- LMDB open database file error
  */
int run
(
		Config *config
);

/**
  * Return 0- success
  *        1- config is not initialized yet
  */
int stop
(
		Config *config
);

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

void runner()
{
	if (!config)
	{
		LOG(ERROR) << "config corrupted.";
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

