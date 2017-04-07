/*
 * oauth2.cpp
 *
 */
#include "oauth2.h"
#include <iostream>
#include <algorithm>
#include <json/json.h>
#include <glog/logging.h>
#include <curl/curl.h>

#include "cppcodec/base64_url.hpp"

#include "utilstring.h"
#include "sslhelper.h"

#define GOOGLE_TOKEN_URL "https://www.googleapis.com/oauth2/v4/token"

static size_t write_string(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string curl_post
(
		const std::string &url,
		const std::string &data
)
{
	std::string r;
	// In windows, this will init the winsock stuff
	curl_global_init(CURL_GLOBAL_ALL);
	// get a curl handle
	CURL *curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
	    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
	    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
	    CURLcode res = curl_easy_perform(curl);
	    if (res != CURLE_OK)
	      LOG(ERROR) << "Error post data: " << curl_easy_strerror(res);
	    curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return r;
}

/**
  * @param pem_pkey
  * @param scope https://www.googleapis.com/auth/spreadsheets https://www.googleapis.com/auth/plus.me
  *	@param service_account accounts.google.com, https://accounts.google.com. 
  * @param audience app's client IDs 
  * @param expires 3600
  */
static std::string getJWT
(
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	int expires
)
{
	Json::Value jwt_header;
    Json::Value jwt_claim_set;
    time_t t = time(NULL);
    Json::FastWriter jfw;
    Json::StyledWriter jsw;

    // Create jwt header
    jwt_header["alg"] = "RS256";
    jwt_header["typ"] = "JWT";

    // jwt claim set
    if (!subject_email.empty())
    	jwt_claim_set["sub"] = subject_email;
    jwt_claim_set["iss"] = service_account;
    jwt_claim_set["scope"] = scope;
    // intended target of the assertion for an access token
    jwt_claim_set["aud"] = audience;
    jwt_claim_set["iat"] = Json::UInt64(t);
    jwt_claim_set["exp"] = Json::UInt64(t + expires);

    // header base64url
    // for header
    std::string header = jfw.write(jwt_header);
    std::string base64url_header = cppcodec::base64_url::encode(header);

	// claim set
	std::string claim_set= jfw.write(jwt_claim_set);
    std::string base64url_claim_set = cppcodec::base64_url::encode(claim_set);

	std::string jwt_b64 = base64url_header + "." + base64url_claim_set;
    // for signature

    std::string jwt_signature;
    int c = jws_sign(jwt_b64, pemkey, jwt_signature);
    if (c != 0)
    {
    	LOG(ERROR) << "Error " << c << ": " << jwt_signature;
    	return "";
    }
    jwt_b64 = jwt_b64 + "." + cppcodec::base64_url::encode(jwt_signature);
    return jwt_b64;
}

std::string loadGoogleToken
(
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	int expires
)
{
	std::string jwt = getJWT(service_account, subject_email, pemkey, scope, audience, expires);
	if (jwt.empty())
		return "";
	std::string json = curl_post(GOOGLE_TOKEN_URL,
				"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt);
	Json::Reader reader;
	Json::Value v;

	bool r = reader.parse(json, v, false);
	if (!r)
	{
	   	LOG(ERROR) << "Failed to parse " << json;
	    return "";
	}
	return v.get("access_token", "UTF-8").asString();
}
