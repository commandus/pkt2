/**
 *   Read protobuf messages from nanomsg socket ipc:///tmp/packet.pkt2 (-q)
 *   Send notification to the FireBase Cloud Messaging
 *
 *   Usage (default values):
 *   	pkt2dumpfcm -q ipc:///tmp/packet.pkt2 -k <token>
 *
 *
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include <glog/logging.h>
#include <libpq-fe.h>

#include <curl/curl.h>

#include "platform.h"
#include "daemonize.h"
#include "pkt2dumpfcm-config.h"
#include "fcmdumper.h"

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

	config = new Config(argc, argv);
	if (!config)
		exit(3);

    if (config->error() != 0)
	{
		std::cerr << ERR_COMMAND;
		exit(config->error());
	}

    INIT_LOGGING(PROGRAM_NAME)

	// check database connection
	PGconn *conn = dbconnect(config);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		LOG(ERROR) << ERR_DATABASE_NO_CONNECTION;
		exit(ERRCODE_DATABASE_NO_CONNECTION);
	}
	PQfinish(conn);

	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);
	
	if (!config->test_data_hex.empty()) {
		sendNotifications(config->test_data_hex, config);
	}

	if (config->daemonize)
	{
		LOG(INFO) << MSG_DAEMONIZE;
		Daemonize daemonize(PROGRAM_NAME, config->path, runner, stopNWait, done, config->max_fd);
	}
	else
	{
		LOG(INFO) << MSG_START;
		if (config->max_fd > 0)
			Daemonize::setFdLimit(config->max_fd);
		runner();
		done();
	}

	return reslt;
}

