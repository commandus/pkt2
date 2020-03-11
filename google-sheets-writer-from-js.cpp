/**
 * google-sheets-writer-from-js.cpp
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <ostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <glog/logging.h>

#include "google-sheets-writer-from-js.h"
#include "errorcodes.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"

#include "utilstring.h"

class JsonHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, JsonHandler> {
public:
	std::vector<std::string> &row;
	std::vector<std::string> &keys;
	JsonHandler(
		std::vector<std::string> &arow,
		std::vector<std::string> &akeys
	) 
		: row(arow), keys(akeys)
	{

	}

    bool Null() { 
		row.push_back("");
		return true; 
	}

    bool Bool(bool b) {
		std::stringstream ss;
		ss << std::boolalpha << b;
		row.push_back(ss.str());
		return true; 
	}

    bool Int(int i) {
		std::stringstream ss;
		ss << i;
		row.push_back(ss.str());
		return true; 
	}

	bool Uint(unsigned u) {
 		std::stringstream ss;
		ss << u;
		row.push_back(ss.str());
		return true; 
	}

    bool Int64(int64_t i) {
		std::stringstream ss;
		ss << i;
		row.push_back(ss.str());
		return true; 
	}

    bool Uint64(uint64_t u) {
		std::stringstream ss;
		ss << u;
		row.push_back(ss.str());
		return true; 
	}

    bool Double(double d) {
		std::stringstream ss;
		ss << d;
		row.push_back(ss.str());
		return true; 
	}

    bool String(const char* str, rapidjson::SizeType length, bool copy) { 
		row.push_back(std::string(str, length));
		return true; 
    }

    bool StartObject() {
		return true; 
	}

    bool Key(const char* str, rapidjson::SizeType length, bool copy) {
		keys.push_back(std::string(str, length));
        return true;
    }

    bool EndObject(rapidjson::SizeType memberCount) {
		return true; 
	}
    bool StartArray() {
		return true; 
	}
    bool EndArray(rapidjson::SizeType elementCount) {
		return true; 
	}
};

int format_number;

/**
 * @brief Print packet to the Google Sheet
 * @param messageTypeNAddress message type
 * @param message message
 * @return 0 success
 */
int put
(
	Config *config,
	const std::string &value
)
{
	if (config->verbosity > 1)
		std::cerr << value;

	rapidjson::Reader reader;
    rapidjson::StringStream ss(value.c_str());
	std::vector<std::string> row, keys;
	JsonHandler handler(row, keys);
    reader.Parse(ss, handler);
	if (reader.HasParseError()) {
    	return ERRCODE_PACKET_PARSE;
	}
	
	int columns = row.size();
	ValueRange newcells;
	newcells.range = GoogleSheets::A1(config->sheet, 0, 0, columns - 1, 0);
	
	if (config->verbosity) {
		for (int i = 0; i < keys.size(); i++)
		{
			std::cerr << keys[i] << " ";
		}
	}
	newcells.values.push_back(row);

	if (!config->google_sheets->append(newcells))
		return ERRCODE_GOOGLE_SHEET_WRITE;	

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
    int fdin = -1;
	if (fin)
		fdin = fileno(fin);
    if (fdin < 0)
    {
        LOG(ERROR) << ERR_NN_BIND << config->filename_input << " " << errno << ": " << strerror(errno);;
		return ERRCODE_NN_BIND;
    }

	void *data = malloc(config->buffer_size);
	while (!config->stop_request)
	{
		size_t packet_length = read(fdin, data, config->buffer_size);
		if (packet_length == 0)
			break;
		std::string s((char *) data, packet_length);

    int code = put(config, s);
    if (code) {
      LOG(ERROR) << ERR_GOOGLE_SHEET_WRITE;
    } else {
      if (config->verbosity >= 1) {
        LOG(INFO) << MSG_SENT;
      }
		}
	}

	free(data);

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
