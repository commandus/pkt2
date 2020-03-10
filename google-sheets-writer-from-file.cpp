/**
 * google-sheets-writer-ffrom-file.cpp
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include <glog/logging.h>

#include <google/protobuf/message.h>

#include "google-sheets-writer-from-file.h"
#include "output-message.h"

#include "errorcodes.h"
#include "utilprotobuf.h"

#include "json/json.h"
#include "pbjson.hpp"

#include "pkt2optionscache.h"

#include "messagedecomposer.h"
#include "utilstring.h"
#include "fieldnamevalueindexstrings.h"

#include "str-pkt2.h"

using namespace google::protobuf;

int format_number;

/**
 * @brief MessageDecomposer callback. Use in conjunction with FieldNameValueIndexStrings class(see first parameter).
 * @param env accumulates field names and values in the InsertSQLStrings object
 * @param message_descriptor message
 * @param field_type type of the data
 * @param field_name field name
 * @param value pointer to the data
 * @param size  size occupied by data
 *
 * @see FieldNameValueIndexStrings
 */
void addFieldValueString2
(
	MessageDecomposer *decomposer,
	void *env,
	const google::protobuf::Descriptor *message_descriptor,
	const google::protobuf::FieldDescriptor *field,
	void* value,
	int size,
	int index
)
{
	((FieldNameValueIndexStrings *) env)->add(field, decomposer->toString(message_descriptor, field, value, size, format_number), index, false);
}

/**
 * @brief Print packet to the stdout as CSV
 * @param messageTypeNAddress message type
 * @param message message
 * @return 0 success
 */
int put
(
	Config *config,
	Pkt2OptionsCache *options,
	MessageTypeNAddress *messageTypeNAddress,
	const google::protobuf::Message *message
)
{
	FieldNameValueIndexStrings vals(options, messageTypeNAddress->message_type);
	MessageDecomposer md(&vals, messageTypeNAddress->message_type, options, message, addFieldValueString2);
	std::cout << vals.toStringTab();
	
	int count = vals.values.size();
	ValueRange newcells;
	newcells.range = GoogleSheets::A1(config->sheet, 0, 0, count - 1, 0);
	
	std::vector<std::string> row;
	for (int i = 0; i < count; i++)
	{
		row.push_back(vals.values[i].value);
	}
	newcells.values.push_back(row);

	if (!config->google_sheets->append(newcells))
	{
		std::cerr << "Error append data";
	}

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
	// IN file
	FILE *fin;
	if (config->filename_input.empty()) {
		fin = stdin;
	} else {
		fin = fopen(config->filename_input.c_str(), "r");
	}
    int fdin = fileno(fin);
    if (fdin < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->filename_input << " " << errno << ": " << strerror(errno);;
		return ERRCODE_NN_BIND;
    }

	EnvPkt2 *strpkt2env = (EnvPkt2 *) initPkt2(config->proto_path, config->verbosity);

	void *data = malloc(config->buffer_size);
	while (!config->stop_request)
	{
		size_t packet_length = read(fdin, data, config->buffer_size);
		if (packet_length == 0)
			break;
		if (config->allowed_packet_sizes.size())
		{
			if (std::find(config->allowed_packet_sizes.begin(), config->allowed_packet_sizes.end(), packet_length) == config->allowed_packet_sizes.end()) {
				LOG(INFO) << MSG_PACKET_REJECTED << packet_length;
				continue;
			}
		}
		std::string s((char *) data, packet_length);

		google::protobuf::Message *message;
		if (parsePacket2Message(&message, strpkt2env, config->input_mode, s, config->force_message)) {
			if (config->allowed_messages.size())
			{
				if (std::find(config->allowed_messages.begin(), config->allowed_messages.end(), message->GetTypeName()) == config->allowed_messages.end())
				{
					LOG(INFO) << MSG_PACKET_REJECTED << message->GetTypeName();
					continue;
				}
			}
			MessageTypeNAddress mta(message->GetTypeName());
			int code = put(config, strpkt2env->options_cache, &mta, message);
			if (code) {
				LOG(ERROR) << ERR_GOOGLE_SHEET_WRITE;
			} else {
				if (config->verbosity >= 1) {
					LOG(INFO) << MSG_SENT;
				}
			}
		}
		if (message)
			delete message;
	}

	free(data);
	donePkt2(strpkt2env);

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
