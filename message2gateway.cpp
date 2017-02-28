/**
 * Read packet from nanomsg socket, parse and emit
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include <glog/logging.h>

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
                LOG(INFO) << MSG_INTERRUPTED;
                stopNWait();
                done();
                break;
        default:
                LOG(INFO) << MSG_SIGNAL;
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
		exit(ERRCODE_NO_CONFIG);
    if (config->error() != 0)
	{
		LOG(ERROR) << ERR_COMMAND;
		exit(ERRCODE_COMMAND);
	}

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, runner, stopNWait, done);
	}
	else
	{
		LOG(INFO) << MSG_START;
		runner();
		done();
	}

	return reslt;
}
