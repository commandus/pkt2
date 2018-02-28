/**
 *   Read protobuf messages from nanomsg socket ipc:///tmp/output.pkt2 (-q)
 *   Print to FireBase messaging
 *
 *   Usage (default values):
 *   	handler-fcm -q ipc:///tmp/packet.pkt2 -m 0
 *
 *   Options
 *   	-m 0- JSON
 *
 *   Error codes:
 *           0- success
 *
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <glog/logging.h>

#include <curl/curl.h>

#include "platform.h"
#include "daemonize.h"
#include "handler-fcm-config.h"
#include "fcm-writer.h"

#include "utilstring.h"

#include "errorcodes.h"

Config *config;

void stopNWait()
{
	if (config)
		stop(config);
}

void done()
{
	curl_global_cleanup();
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
 * @return 0
 * @see errorcodes.h
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

	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_CONFIG);
    if (config->error() != 0)
	{
		LOG(ERROR) << getErrorDescription(config->error());
		exit(config->error());
	}

	// check database connection
	PGconn *conn = dbconnect(config);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		LOG(ERROR) << ERR_DATABASE_NO_CONNECTION;
		exit(ERRCODE_DATABASE_NO_CONNECTION);
	}
	PQfinish(conn);

    INIT_LOGGING(PROGRAM_NAME)

	if (config->verbosity >= 2)
	{
		LOG(INFO) << "arguments: " << arg2String(argc, argv);
	}

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, config->path, runner, stopNWait, done, 0);
	}
	else
	{
		LOG(INFO) << MSG_START;
		runner();
		done();
	}

	return reslt;
}
