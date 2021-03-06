/**
 *   Check pkt2receiver state
 *
 *   Usage (default values):
 *   	pkt2receiver-check -c ipc:///tmp/control.pkt2
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
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#define close closesocket
#else
#endif

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include "platform.h"
#include "daemonize.h"
#include "pkt2receiver-check.h"
#include "pkt2receiver-check-config.h"

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
}

int reslt;

void runner()
{
	if (!config)
	{
		LOG(ERROR) << ERR_NO_CONFIG;
		return;
	}
	config->stop_request = 0;
	int n = 0;
	while (!config->stop_request)
	{
		LOG(INFO) << MSG_START << " " << n + 1;
//		reslt = run(config);
		LOG(INFO) << MSG_STOP;
		if (n >= config->retries)
			break;
		SLEEP(config->retry_delay);
		n++;
	}
	LOG(INFO) << MSG_DONE;
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
		std::cerr << MSG_SIGNAL << signal;
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
#if defined(_WIN32) || defined(_WIN64)
#else
    setSignalHandler(SIGINT);
	setSignalHandler(SIGHUP);
#endif
	reslt = 0;

	config = new Config(argc, argv);
	if (!config)
		exit(ERRCODE_CONFIG);
    if (config->error() != 0)
	{
		LOG(ERROR) << getErrorDescription(config->error());
		exit(config->error());
	}

    INIT_LOGGING(PROGRAM_NAME)

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

//-------------------------------------------------------------------------

/**
 * @brief Write line loop
 * @param config configuration
 * @return  0- success
 *          >0- error (see errorcodes.h)
 */
int run
(
	Config *config
)
{
	START:
	config->stop_request = 0;
	int control_socket = nn_socket(AF_SP, NN_BUS);
	if (control_socket < 0)
	{
		LOG(ERROR) << ERR_NN_SOCKET << config->control_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SOCKET;
	}
	int eid = nn_connect(control_socket, config->control_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->control_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY;
		return ERRCODE_NO_MEMORY;
	}
	LOG(INFO) << MSG_START << " success, main loop.";
	while (!config->stop_request)
    {
    	int bytes = nn_recv(control_socket, buffer, config->buffer_size, 0);
    	if (bytes < 0)
    	{
			if (errno == EINTR) 
			{
				LOG(ERROR) << ERR_INTERRUPTED;
				config->stop_request = true;
				break;
			}
			else
				LOG(ERROR) << ERR_NN_RECV << errno << " " << strerror(errno);
    		continue;
    	}
		if (config->verbosity >= 2)
		{
			LOG(INFO) << MSG_RECEIVED << bytes << ": " << pkt2utilstring::hexString(buffer, bytes) ;
		}
		std::string s((char *)buffer, bytes);

		std::vector<std::string> r = pkt2utilstring::split(s, '\t');
		if (r.size() >= 5)
		{
			uint64_t session_id = atoll(r[0].c_str());
			uint64_t count_packet_in = atoll(r[1].c_str());
			uint64_t count_packet_out = atoll(r[2].c_str());
			int32_t typ = atoll(r[3].c_str());
			uint64_t sz = atoll(r[4].c_str());
			std::string msg = r[5];
			std::string payload = r[6];
			std::cout << count_packet_in << "/" << count_packet_out << std::endl;
		}
    }

    free(buffer);

	int r = nn_shutdown(control_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->control_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}

	close(control_socket);
	control_socket = 0;

	if (config->stop_request == 2)
		goto START;
	
	return r;
}

/**
 *  @brief Stop writer
 *  @param config
 *  @return 0- success
 *          >0- config is not initialized yet
 */
int stop
(
		Config *config
)
{
    if (!config)
    {
    	LOG(ERROR) << ERR_STOP;
        return ERRCODE_STOP;
    }
    config->stop_request = 1;
    return ERR_OK;
}

int reload(Config *config)
{
	if (!config)
		return ERRCODE_NO_CONFIG;
	LOG(ERROR) << MSG_RELOAD_BEGIN;
	config->stop_request = 2;
	return ERR_OK;
}
