/**
 *   repeator
 *
 *   Usage (default values):
 *   	repeator -i ipc:///tmp/control.pkt2 -o tcp://0.0.0.0:50000
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

#include <nanomsg/nn.h>
#include <nanomsg/bus.h>

#include "platform.h"
#include "daemonize.h"
#include "repeator-config.h"

#include "utilstring.h"
#include "errorcodes.h"

Config *config;

void done()
{
	LOG(INFO) << MSG_DONE;
}

int reslt;

/**
 *  @brief Stop writer
 *  @param config program config
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
	int in_socket = nn_socket(AF_SP, NN_BUS);
	if (in_socket < 0)
	{
		LOG(ERROR) << ERR_NN_SOCKET << config->in_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_SOCKET;
	}
	int eid = nn_connect(in_socket, config->in_url.c_str());
	if (eid < 0)
	{
		LOG(ERROR) << ERR_NN_CONNECT << config->in_url << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_CONNECT;
	}

	void *buffer = malloc(config->buffer_size);
	if (!buffer)
	{
		LOG(ERROR) << ERR_NO_MEMORY;
		return ERRCODE_NO_MEMORY;
	}

	std::vector<int> eids;

	std::vector<int> out_sockets;
	out_sockets.resize(config->out_urls.size());
	eids.resize(config->out_urls.size());
	for (int i = 0; i < config->out_urls.size(); i++) 
	{
		out_sockets[i] = nn_socket(AF_SP, NN_BUS);
		if (out_sockets[i] < 0)
		{
			LOG(ERROR) << ERR_NN_SOCKET << config->out_urls[i] << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_SOCKET;
		}
		eids[i] = nn_bind(out_sockets[i], config->out_urls[i].c_str());
		if (eids[i] < 0)
		{
			LOG(ERROR) << ERR_NN_BIND << out_sockets[i] << " " << errno << " " << strerror(errno);
			return ERRCODE_NN_CONNECT;
		}
		
		if (config->verbosity >= 2)
		{
			LOG(INFO) << "Bind socket " << out_sockets[i] << "/" << eid << " to " << config->out_urls[i];
		}
	}	
	
	LOG(INFO) << MSG_START << " success, main loop.";
	
	while (!config->stop_request)
	{
		int bytes = nn_recv(in_socket, buffer, config->buffer_size, 0);
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
			LOG(INFO) << MSG_RECEIVED << bytes << ": " << hexString(buffer, bytes) ;
		}
		for (int i = 0; i < eids.size(); i++)
		{
			int r = nn_send(out_sockets[i], buffer, bytes, 0);
			if (r)
			{
				LOG(ERROR) << ERR_NN_SEND << config->out_urls[i] << " " << errno << " " << strerror(errno);
				r = ERRCODE_NN_SEND;
			}
			else
			{
				if (config->verbosity >= 2)
				{
					LOG(INFO) << MSG_SENT << bytes << " to : " << config->out_urls[i];
				}
			}
		}
	}
	free(buffer);

	int r = nn_shutdown(in_socket, eid);
	if (r)
	{
		LOG(ERROR) << ERR_NN_SHUTDOWN << config->in_url << " " << errno << " " << strerror(errno);
		r = ERRCODE_NN_SHUTDOWN;
	}

	for (int i = 0; i < eids.size(); i++)
	{
		int r = nn_shutdown(out_sockets[i], eids[i]);
		if (r)
		{
			LOG(ERROR) << ERR_NN_SHUTDOWN << i << ")" << out_sockets[i] << "/" << eids[i] << " " << config->out_urls[i] << " " << errno << " " << strerror(errno);
			r = ERRCODE_NN_SHUTDOWN;
		}
		else
		{
			if (config->verbosity >= 2)
			{
				LOG(INFO) << MSG_SHUTDOWN << i << ") " << out_sockets[i] << "/" << eids[i] << " " << config->out_urls[i];
			}
		}
	}
	
	close(in_socket);

	if (config->stop_request == 2)
		goto START;
	
	return r;
}

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
		LOG(INFO) << MSG_START << " # " << n + 1;
		reslt = run(config);
		LOG(INFO) << MSG_STOP;
		if (n >= config->retries)
			break;
		n++;
		sleep(config->retry_delay);
	}
	LOG(INFO) << MSG_DONE;
}

void stopNWait()
{
	LOG(INFO) << MSG_STOP;
	if (config)
		stop(config);
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
