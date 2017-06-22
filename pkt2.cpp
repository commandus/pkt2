#include <stdio.h>
#include <signal.h>

#include <iostream>
#include <glog/logging.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>

#include "platform.h"
#include "daemonize.h"
#include "pkt2-config.h"
#include "pkt2-impl.h"
#include "errorcodes.h"

Config *config;

void stopNWait()
{
    LOG(INFO) << MSG_STOP;
	stop(config);
}

void done()
{
    LOG(INFO) << MSG_DONE;
}

int reslt;

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGTERM:
		std::cerr << MSG_TERMINATE;
		stopNWait();
		LOG(ERROR) << MSG_TERMINATE;
		// done();
		{
			int saved_errno = errno;
			while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) 
			{
				std::cerr << MSG_CHILD_WAITING << std::endl;
				sleep(1);
			}
			std::cerr << MSG_CHILD_TERMINATED << std::endl;
			LOG(ERROR) << MSG_CHILD_TERMINATED;
			errno = saved_errno;
		}
		std::cerr << MSG_TERMINATED;
		LOG(ERROR) << MSG_TERMINATED;
		break;
	case SIGINT:
		std::cerr << MSG_INTERRUPTED;
		stopNWait();
		// done();
		break;
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST;
		reload(config);
		break;
	case SIGCHLD:
		{
			int saved_errno = errno;
			while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) 
			{
				std::cerr << MSG_CHILD_WAITING << std::endl;
				sleep(1);
			}
			std::cerr << MSG_CHILD_TERMINATED << std::endl;
			errno = saved_errno;
		}
		break;
	default:
		std::cerr << MSG_SIGNAL << signal;
	}
}

void setSignalHandler(int signal, int flags)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	action.sa_flags = flags;
	sigaction(signal, &action, NULL);
}

void run()
{
	if (!config)
	{
		LOG(ERROR) << ERR_PARSE_COMMAND;
		return;
	}

	// Signal handlers
	setSignalHandler(SIGINT, 0);
	setSignalHandler(SIGTERM, 0);
	setSignalHandler(SIGHUP, 0);
	setSignalHandler(SIGCHLD, SA_RESTART | SA_NOCLDSTOP);

	while (!config->stop_request)
	{
		reslt = pkt2(config);
		if (reslt == ERRCODE_CONFIG)
		{
			LOG(ERROR) << ERR_CONFIG;
			break;
		}
	}
}

int main
(
	int argc, 
	char *argv[]
)
{
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
