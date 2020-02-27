#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <glog/logging.h>

#include "pkt2js-impl.h"
#include "str-pkt2.h"

#include "errorcodes.h"

/**
  * @return: 0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
  */
int pkt2js(Config *config)
{
	config->session_id = 0;
	config->count_packet_in = 0;
	config->count_packet_out = 0;
START:
	config->stop_request = 0;

	// IN socket
	FILE *fin;
	if (config->filenameInput.empty()) {
		fin = stdin;
	} else {
		fin = fopen(config->filenameInput.c_str(), "r");
	}
    int fdin = fileno(fin);
    if (fdin < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->filenameInput << " " << errno << ": " << strerror(errno);;
		return ERRCODE_NN_BIND;
    }

	// OUT socket
	FILE *fout;
	if (config->filenameOutput.empty()) {
		fout = stdout;
	} else {
		fout = fopen(config->filenameInput.c_str(), "r");
	}
    int fdout = fileno(fout);
    if (fdout < 0) {
		LOG(ERROR) << ERR_NN_SOCKET << config->filenameOutput << " " << errno << " " << strerror(errno);
		return ERRCODE_NN_BIND;
    }

	void *strpkt2env = initPkt2(config->proto_path, config->verbosity);

	void *data = malloc(config->buffer_size);
	while (!config->stop_request)
	{
		config->session_id++;
		size_t packet_length = read(fdin, data, config->buffer_size);
		if (packet_length == 0)
			break;
		config->count_packet_in++;
		if (config->allowed_packet_sizes.size())
		{
			if (std::find(config->allowed_packet_sizes.begin(), config->allowed_packet_sizes.end(), packet_length) == config->allowed_packet_sizes.end()) {
				LOG(INFO) << MSG_PACKET_REJECTED << packet_length;
				continue;
			}
		}
		std::string s((char *) data, packet_length);
		std::string outstr = parsePacket(strpkt2env, config->input_mode, config->output_mode, s, config->force_message);
		if (!outstr.empty()) {
			int sent = write(fdout, outstr.c_str(), outstr.size());

			if (sent < 0) {
				LOG(ERROR) << ERR_NN_SEND << sent;
			}
			else {
				config->count_packet_out++;
				if (config->verbosity >= 1) {
					LOG(INFO) << MSG_SENT << sent << std::endl;
				}
			}
		}
	}

	free(data);
	donePkt2(strpkt2env);

	if (fdout) {
		fclose(fout);
		fout = NULL;
		fdout = 0;
	}

	if (fdin) {
		fclose(fin);
		fin = NULL;
		fdin = 0;
	}

	if (config->stop_request == 2)
		goto START;

   	return ERR_OK;
}

/**
  * @return 0- success
  *         ERRCODE_STOP- config is not initialized yet
  */
int stop(Config *config)
{
    if (!config)
        return ERRCODE_STOP;
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
