/*
 * google-sheets.cpp
 *
 */

#include "google-sheets.h"
#include <iostream>
#include <vector>
#include <algorithm>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include <json/json.h>
// base64url
#include "cppcodec/base64_url.hpp"

#include "utilstring.h"

#define GOOGLE_TOKEN_URL		"https://www.googleapis.com/oauth2/v4/token"
#define API_SHEET				"https://sheets.googleapis.com/v4/spreadsheets/"

enum class CSVState 
{
	UnquotedField,
	QuotedField,
	QuotedQuote
};

static std::vector<std::string> readCSVRow
(
	const std::string &row
)
{
    CSVState state = CSVState::UnquotedField;
    std::vector<std::string> fields {""};
    size_t i = 0; // index of the current field
    for (char c : row) 
    {
        switch (state) {
            case CSVState::UnquotedField:
                switch (c) {
                    case ',': // end of field
                              fields.push_back(""); i++;
                              break;
                    case '"': state = CSVState::QuotedField;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedField:
                switch (c) {
                    case '"': state = CSVState::QuotedQuote;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedQuote:
                switch (c) {
                    case ',': // , after closing quote
                              fields.push_back(""); i++;
                              state = CSVState::UnquotedField;
                              break;
                    case '"': // "" -> "
                              fields[i].push_back('"');
                              state = CSVState::QuotedField;
                              break;
                    default:  // end of quote
                              state = CSVState::UnquotedField;
                              break; }
                break;
        }
    }
    return fields;
}

/** Read CSV file
  * http://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
  */
static void readCSV
(
	std::istream &in,
	std::vector<std::vector<std::string>> &retval 
)
{
	std::string row;
	while (true) 
	{
		std::getline(in, row);
		if (in.bad() || in.eof())
			break;
		retval.push_back(readCSVRow(row));
    }
}

static size_t write_string(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
  * @brief POST data, return received data in retval
  * Return 0- success, otherwise error code. retval contains error description
  */
static CURLcode curl_post
(
	const std::string &url,
	const std::string &data,
	std::string &retval
)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return CURLE_FAILED_INIT; 
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &retval);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
		retval = curl_easy_strerror(res);
	curl_easy_cleanup(curl);
	return res;
}

CURLcode curl_get
(
	const std::string &token,
	const std::string &url,
	std::string &retval
)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return CURLE_FAILED_INIT; 
	struct curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, ("authorization: Bearer " + token).c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &retval);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
		retval = curl_easy_strerror(res);
	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);
	return res;
}

CURLcode curl_put
(
	const std::string &token,
	const std::string &url,
	const std::string &data,
	std::string &retval
)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return CURLE_FAILED_INIT;
	struct curl_slist *chunk = NULL;
	chunk = curl_slist_append(chunk, ("authorization: Bearer " + token).c_str());
	chunk = curl_slist_append(chunk, "Content-Type: application/json");
	
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	
	
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &retval);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    CURLcode res = curl_easy_perform(curl);
  
    if (res != CURLE_OK)
		retval = curl_easy_strerror(res);
	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);
	return res;
}

/**
 * If error occurred, retval contains error descrtiption
 * @param data
 * @param pemkey PEM private key
 * @param retval
 * @return
 */
int jws_sign
(
	const std::string &data,
	const std::string &pemkey,
	std::string &retval
)
{
    SHA256_CTX mctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Init(&mctx);
    SHA256_Update(&mctx, data.c_str(), data.size());
    SHA256_Final(hash, &mctx);

	unsigned char *sig;
	size_t slen;

	EVP_PKEY *key;
	EVP_PKEY_CTX *kctx;
		
	BIO *pem = BIO_new_mem_buf((void *) pemkey.c_str(), (int) pemkey.length());
	if (pem == NULL)
		goto err;
	key = PEM_read_bio_PrivateKey(pem, NULL, NULL, (void *) "");
    
    kctx = EVP_PKEY_CTX_new(key, NULL);
    if (!kctx)
    	goto err;
    if (EVP_PKEY_sign_init(kctx) <= 0)
    	goto err;
    if (EVP_PKEY_CTX_set_rsa_padding(kctx, RSA_PKCS1_PADDING) <= 0)
    	goto err;
    if (EVP_PKEY_CTX_set_signature_md(kctx, EVP_sha256()) <= 0)
    	goto err;
    // Determine buffer length
    slen = 0;
    if (EVP_PKEY_sign(kctx, NULL, &slen, hash, SHA256_DIGEST_LENGTH) <= 0)
    	goto err;
    sig = (unsigned char *) OPENSSL_malloc(slen);
    if (!sig)
    	goto err;
    if (EVP_PKEY_sign(kctx, sig, &slen, hash, SHA256_DIGEST_LENGTH) <= 0)
    	goto err;
    retval = std::string((char *) sig, (unsigned int) slen);
    return 0;
err:
    // Clean up
    EVP_cleanup();
    int err = ERR_peek_error();
    const char *r = ERR_error_string(err, NULL);
    if (r) 
    	retval = std::string(r);
   	else
		retval = "Error";    	
    return err;
}

/**
  *	@brief Generate JWT- base64url encoded JWT header
  * @param service_account accounts.google.com, https://accounts.google.com.
  *	@param subject_email  
  * @param pemkey
  * @param scope https://www.googleapis.com/auth/spreadsheets https://www.googleapis.com/auth/plus.me
  * @param audience app's client IDs 
  * @param expires 3600
  * @param retval
  */
static int getJWT
(
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	int expires,
	std::string &retval
)
{
	Json::Value jwt_header;
    Json::Value jwt_claim_set;
    time_t t = time(NULL);
    Json::FastWriter jfw;

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

	retval = base64url_header + "." + base64url_claim_set;

    // for signature
    std::string jwt_signature;
    int c = jws_sign(retval, pemkey, jwt_signature);
    if (c != 0)
    {
    	retval = jwt_signature;
    	return c;
    }
    retval += "." + cppcodec::base64_url::encode(jwt_signature);
    return 0;
 }

/**
  *	@brief Send JWT to the 
  * @param service_account accounts.google.com, https://accounts.google.com.
  *	@param subject_email  
  * @param pemkey
  * @param scope https://www.googleapis.com/auth/spreadsheets https://www.googleapis.com/auth/plus.me
  * @param audience app's client IDs 
  * @param expires 3600
  */
int loadGoogleToken
(
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	int expires,
	std::string &retval
)
{
	std::string jwt;
	int r = getJWT(service_account, subject_email, pemkey, scope, audience, expires, jwt);
	if (r != 0)
	{
		retval = jwt;
		return r;
	}
	std::string json;
	CURLcode res = curl_post(GOOGLE_TOKEN_URL,
			"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt, json);
	if (res != CURLE_OK)
	{
		retval = curl_easy_strerror(res);
		return res;
	}
	
	Json::Reader reader;
	Json::Value v;
	if (reader.parse(json, v))
	{
	    retval = v.get("access_token", "UTF-8").asString();
	}
    else
    {
	    retval = "Parse error " + json;
	 	r = -1;
	 }
	return r;
}

/**
  * @brief load Google service token bearer from rhe Google service JSON object
  */
bool readGoogleTokenJSON
(
	const std::string &json,
	std::string &ret_service_account,
	std::string &ret_pemkey
)
{
	Json::Reader reader;
	Json::Value v;

	if (json.empty() || (!reader.parse(json, v)))
		return false;		
	
	ret_service_account = v["client_id"].asString();
	ret_pemkey = v["private_key"].asString();
	size_t start_pos = 0;
    while((start_pos = ret_pemkey.find("\\n", start_pos)) != std::string::npos) 
    {
        ret_pemkey.replace(start_pos, 2, "\n");
        start_pos++;
    }
    return true;
}

int ValueRange::parseJSON(const std::string &json)
{
	Json::Reader reader;
	Json::Value v;

	values.clear();
	
	if (!reader.parse(json, v))
	{
		return -1;		
	}
	range = v["range"].asString();
	major_dimension = v["majorDimension"].asString();
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
	return 0;
}

ValueRange::ValueRange()
	: major_dimension("ROWS")
{
	
}

ValueRange::ValueRange
(
	const std::string &rang, 
	std::istream &stream)
	: major_dimension("ROWS"), range(rang)
{
	values.clear();
	readCSV(stream, values);
}

int ValueRange::parseCSV(const std::string &csv)
{
	values.clear();
	std::stringstream stream(csv);
	readCSV(stream, values);
	return 0;
}

/**
  * @brief debug print
  */
std::string ValueRange::toString() const
{
	std::stringstream ss;
	ss << range << " " << major_dimension << std::endl;
	for (std::vector<std::vector<std::string>>::const_iterator itr(values.begin()); itr != values.end(); ++itr)
    {
    	std::vector<std::string> row = *itr;
    	for (std::vector<std::string>::const_iterator itc(itr->begin()); itc != itr->end(); ++itc)
    	{
    		ss << *itc << " ";
    	}
    	ss << std::endl;
    }
	return ss.str();	
}

std::string ValueRange::toJSON() const
{
    Json::Value root;
	root["range"] = range;
	root["majorDimension"] = major_dimension;
	Json::Value rows = Json::Value(Json::arrayValue);
	for (std::vector<std::vector<std::string>>::const_iterator itr(values.begin()); itr != values.end(); ++itr)
    {
    	Json::Value cols = Json::Value(Json::arrayValue);
    	std::vector<std::string> row = *itr;
    	for (std::vector<std::string>::const_iterator itc(itr->begin()); itc != itr->end(); ++itc)
    	{
	    	cols.append(*itc);
		}
		rows.append(cols);
    }
    root["values"] = rows;
    Json::FastWriter jfw;
    return jfw.write(root);
}    


GoogleSheets::GoogleSheets
(
	const std::string sheetid,
	const std::string tokenbearer
)
	: sheet_id(sheetid), token(tokenbearer)
{
}

GoogleSheets::GoogleSheets
(
	const std::string &sheetid,
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience
)
{
	sheet_id = sheetid;	
	genTokenParams.push_back(service_account);
	genTokenParams.push_back(subject_email);
	genTokenParams.push_back(pemkey);
	genTokenParams.push_back(scope);
	genTokenParams.push_back(audience);
	genToken();
}

/**
  * Re-generate token
  */
bool GoogleSheets::genToken()
{
	if (genTokenParams.size() < 5)
		return false;
	loadGoogleToken(
		genTokenParams[0],
		genTokenParams[1],
		genTokenParams[2],
		genTokenParams[3],
		genTokenParams[4],
		3600,
		token
	);
	return true;
}

GoogleSheets::~GoogleSheets() 
{
}

/**
 * @param range Sheet1!A1:D5
 */
int GoogleSheets::getRange
(
	const std::string &range, 
	ValueRange &retval
)
{
	std::string json;
	CURLcode r = curl_get(token, API_SHEET + sheet_id + "/values/" + range, json);
	if (r != CURLE_OK)
		return r;
	int pr = retval.parseJSON(json);
	return pr;
}

/**
 * @param values
 */
int GoogleSheets::putRange
(
	const ValueRange &values
)
{
	std::string json = values.toJSON();
std::cerr << json;	
	std::string response;
	CURLcode r = curl_put(token, API_SHEET + sheet_id + "/values/" + values.range + "?valueInputOption=USER_ENTERED", json, response);
	if (r != CURLE_OK)
		return r;
	return 0;
}


