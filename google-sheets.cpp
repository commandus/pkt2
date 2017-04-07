/*
 * google-sheets.cpp
 *
 */

#include "google-sheets.h"
#include <curl/curl.h>
#include <json/json.h>

#include "utilstring.h"

ValueRange::ValueRange()
{
	
}

ValueRange::~ValueRange()
{
	
}

bool ValueRange::parse(const std::string &json)
{
	Json::Reader reader;
	Json::Value v;

	values.clear();
	
	bool r = reader.parse(json, v, false);
	if (!r)
	    return false;
	for (const Json::Value& row : v["values"])
    {
    	std::vector<std::string> r;
    	for (const Json::Value& cell : row)
    	{
    		std::string s = cell.asString();
        	r.push_back(s);
        }
        values.push_back(r);
    }
	return true;
}

/**
  * @brief debug print
  */
std::string ValueRange::toString()
{
	std::stringstream ss;
	for (std::vector<std::vector<std::string>>::iterator itr(values.begin()); itr != values.end(); ++itr)
    {
    	std::vector<std::string> row = *itr;
    	for (std::vector<std::string>::iterator itc(itr->begin()); itc != itr->end(); ++itc)
    	{
    		ss << *itc << " ";
    	}
    	ss << std::endl;
    }
	return ss.str();	
}

static size_t write_string(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string curl_get
(
	const std::string &token,
	const std::string &url
)
{
	std::string r;
	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);
	// get a curl handle
	CURL *curl = curl_easy_init();
	if (curl) 
	{
		struct curl_slist *chunk = NULL;
		chunk = curl_slist_append(chunk, ("authorization: Bearer " + token).c_str());
		chunk = curl_slist_append(chunk, ":authority: content-sheets.googleapis.com");

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
	    CURLcode res = curl_easy_perform(curl);
	    if (res != CURLE_OK)
			r = "";
	    curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return r;
}

GoogleSheets::GoogleSheets
(
	const std::string sheetid,
	const std::string tokenbearer
)
	: sheet_id(sheetid), token(tokenbearer)
{
}

GoogleSheets::~GoogleSheets() {
	// TODO Auto-generated destructor stub
}

/**
 * @param range Sheet1!A1:D5
 */
int GoogleSheets::getRange(const std::string &range, ValueRange &retval)
{
	// GET https://sheets.googleapis.com/v4/spreadsheets/spreadsheet_id/values/range
	std::string json = curl_get(token, "https://sheets.googleapis.com/v4/spreadsheets/" + sheet_id + "/values/" + range);
	retval.parse(json);
	std::cerr << json << std::endl; 
	return 0;
}

