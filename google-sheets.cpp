/*
 * google-sheets.cpp
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

#include "utilstring.h"

#define GOOGLE_TOKEN_URL		"https://www.googleapis.com/oauth2/v4/token"
#define API_SHEET				"https://sheets.googleapis.com/v4/spreadsheets/"

enum CSVState 
{
	UnquotedField,
	QuotedField,
	QuotedQuote
};

const static unsigned char* b64 = (unsigned char *) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string base64url_encode
(
	const unsigned char* data, 
	size_t data_len
)
{
	std::stringstream r;
	size_t rc = 0;
	size_t modulusLen = data_len % 3;
	size_t pad = ((modulusLen & 1) << 1) + ((modulusLen & 2) >> 1);
	size_t result_len = 4 * (data_len + pad) / 3;
	if (pad == 2) 
		result_len -= 2;
	else 
		if (pad == 1) 
			result_len -= 1;

	std::string result(result_len, '\0');
	size_t byteNo;
	for (byteNo = 0; byteNo + 3 <= data_len; byteNo += 3) 
	{
		unsigned char BYTE0 = data[byteNo];
		unsigned char BYTE1 = data[byteNo + 1];
		unsigned char BYTE2 = data[byteNo + 2];
		result[rc++] = b64[BYTE0 >> 2];
		result[rc++] = b64[((0x3 & BYTE0) << 4) + (BYTE1 >> 4)];
		result[rc++] = b64[((0x0f & BYTE1) << 2) + (BYTE2 >> 6)];
		result[rc++] = b64[0x3f & BYTE2];
	}

	if (pad == 2) 
	{
		result[rc++] = b64[data[byteNo] >> 2];
		result[rc++] = b64[(0x3 & data[byteNo]) << 4];
	} 
	else if (pad == 1) 
	{
		result[rc++] = b64[data[byteNo] >> 2];
		result[rc++] = b64[((0x3 & data[byteNo]) << 4) + (data[byteNo + 1] >> 4)];
		result[rc++] = b64[(0x0f & data[byteNo + 1]) << 2];
	}
	return result;
}

std::string base64_url_encode
(
	const std::string &value
)
{
	return base64url_encode((unsigned  char *) &value[0], value.size());
}

/**
  * @brief parse SCV record
  */
static std::vector<std::string> readCSVRow
(
	const std::string &row
)
{
    CSVState state = UnquotedField;
    std::vector<std::string> fields;
	fields.push_back("");
    size_t i = 0; // index of the current field
    for (int r = 0; r < row.length(); r++)
    {
		char c = row.at(r);
		switch (state) {
		case UnquotedField:
			switch (c) {
			case ',': // end of field
				fields.push_back(""); i++;
				break;
			case '"': 
				state = QuotedField;
				break;
			default:  
				fields[i].push_back(c);
				break;
			}
			break;
		case QuotedField:
			switch (c) {
			case '"': 
				state = QuotedQuote;
				break;
			default:
				fields[i].push_back(c);
				break;
			}
			break;
		case QuotedQuote:
			switch (c) {
			case ',': // , after closing quote
				fields.push_back("");
				i++;
				state = UnquotedField;
				break;
			case '"': // "" -> "
				fields[i].push_back('"');
				state = QuotedField;
				break;
			default:  // end of quote
				state = UnquotedField;
				break;
			}
			break;
		}
	}
	return fields;
}

/** 
  * @brief Read CSV file
  * http://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
  */
static void readCSV
(
	std::istream &in,
	std::vector<std::vector<std::string> > &retval 
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

/**
  * @brief CURL write callback
  */
static size_t write_string(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/**
  * @brief POST data, return received data in retval
  * Return 0- success, otherwise error code. retval contains error description
  */
static int curl_post0
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
	int http_code;
    if (res != CURLE_OK)
	{
		retval = curl_easy_strerror(res);
		http_code = - res;
	}
	else
	{
		// std::cerr << "POST 0 return: " << retval << std::endl;
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	}
	curl_easy_cleanup(curl);
	return http_code;
}

/**
  * @brief GET
  */
int curl_get
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
	int http_code;
    if (res != CURLE_OK)
	{
		retval = curl_easy_strerror(res);
		http_code = res;
	}
	else
	{
		// std::cerr << "GET return: " << retval << std::endl;
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	}
	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);
	return http_code;
}

/**
  * @brief PUT
  */
int curl_put
(
	const std::string &token,
	const std::string &url,
	const std::string &data,
	std::string &retval
)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return - CURLE_FAILED_INIT;
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
  
	int http_code;
    if (res != CURLE_OK)
	{
		retval = curl_easy_strerror(res);
		http_code = - res;
	}
	else
	{
		// std::cerr << "PUT return: " << retval << std::endl;
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	}

	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);
	return http_code;
}

/**
  * @brief POST
  * @param token GWT
  * @param url locator
  * @param data data to POST
  * @return negative number: CURL error code(invert sign to get CURL code), positive number: HTTP code
  */
int curl_post
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
	
	// Print out request
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &retval);
    CURLcode res = curl_easy_perform(curl);
  
	int http_code;
    if (res != CURLE_OK)
	{
		retval = curl_easy_strerror(res);
		http_code = - res;
	}
	else
	{
		// std::cerr << "POST return: " << retval << std::endl;
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	}

	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);
	return http_code;
}

/**
 * @brief JWT signing. If error occurred, retval contains error descrtiption
 * @param data data to sign
 * @param pemkey PEM private key
 * @param retval return value
 * @return 0- success
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
  *	@param subject_email email
  * @param pemkey PEM private key
  * @param scope https://www.googleapis.com/auth/spreadsheets https://www.googleapis.com/auth/plus.me
  * @param audience app's client IDs 
  * @param expires 3600
  * @param retval JWT
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
    std::string base64url_header = base64_url_encode(header);

	// claim set
	std::string claim_set= jfw.write(jwt_claim_set);
    std::string base64url_claim_set = base64_url_encode(claim_set);

	retval = base64url_header + "." + base64url_claim_set;

    // for signature
    std::string jwt_signature;
    int c = jws_sign(retval, pemkey, jwt_signature);
    if (c != 0)
    {
    	retval = jwt_signature;
    	return c;
    }
    retval += "." + base64_url_encode(jwt_signature);
    return 0;
 }

/**
  *	@brief Send JWT to the 
  * @param service_account accounts.google.com, https://accounts.google.com.
  *	@param subject_email email
  * @param pemkey PEM private key
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
	
	int res = curl_post0(GOOGLE_TOKEN_URL,
			"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt, json);
	if (res != 200)
	{
		retval = curl_easy_strerror((CURLcode) res);
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
	std::string &ret_pemkey,
	std::string &email
)
{
	Json::Reader reader;
	Json::Value v;

	if (json.empty() || (!reader.parse(json, v)))
		return false;		
	
	ret_service_account = v["client_id"].asString();
	ret_pemkey = v["private_key"].asString();
	email = v["client_email"].asString();

	size_t start_pos = 0;
    while((start_pos = ret_pemkey.find("\\n", start_pos)) != std::string::npos) 
    {
        ret_pemkey.replace(start_pos, 2, "\n");
        start_pos++;
    }
    return true;
}

/**
  * @brief clear values
  */
void ValueRange::clear()
{
	range = "";
	values.clear();
}

/**
  * @brief parse data to the values
  */
int ValueRange::parseJSON(const std::string &json)
{
	Json::Reader reader;
	Json::Value v;

	clear();
	
	if (!reader.parse(json, v))
	{
		return -1;		
	}
	range = v["range"].asString();
	major_dimension = v["majorDimension"].asString();
	
	for (Json::Value::const_iterator row_it(v["values"].begin()); row_it != v["values"].end(); ++row_it)
    {
    	std::vector<std::string> r;
		for (Json::Value::const_iterator it_cell(row_it->begin()); it_cell != row_it->end(); ++it_cell)
    	{
    		std::string s = it_cell->asString();
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
	: range(rang), major_dimension("ROWS")
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
	for (std::vector<std::vector<std::string> >::const_iterator itr(values.begin()); itr != values.end(); ++itr)
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

/**
  * @brief get JSON string
  */
std::string ValueRange::toJSON() const
{
    Json::Value root;
	root["range"] = range;
	root["majorDimension"] = major_dimension;
	Json::Value rows = Json::Value(Json::arrayValue);
	for (std::vector<std::vector<std::string> >::const_iterator itr(values.begin()); itr != values.end(); ++itr)
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
	const std::string &spreadsheet,
	const std::string tokenbearer,
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	on_token_bearer onTokenbearer,
	void *environ
)
	: env(environ), ontokenbearer(onTokenbearer)
{
	sheet_id = spreadsheet;
	token = tokenbearer;	
	genTokenParams.push_back(service_account);
	genTokenParams.push_back(subject_email);
	genTokenParams.push_back(pemkey);
	genTokenParams.push_back(scope);
	genTokenParams.push_back(audience);
	if (token.empty())
		genToken();
}

GoogleSheets::~GoogleSheets() 
{
}

/**
  * @brief Assign a new token received event handler
  * Use to store token bearer
  */
void GoogleSheets::setOnTokenBearer
(
	void *environ,
	on_token_bearer value
)
{
	env = environ;
	ontokenbearer = value;
}

/**
  * @brief Re-generate token
  */
bool GoogleSheets::genToken()
{
	if (genTokenParams.size() < 5)
	{
		if (ontokenbearer)
			ontokenbearer(env, token, -1);
		return false;
	}
	int r = loadGoogleToken(
		genTokenParams[0],
		genTokenParams[1],
		genTokenParams[2],
		genTokenParams[3],
		genTokenParams[4],
		3600,
		token
	);
	
	if (ontokenbearer)
		ontokenbearer(env, token, r);
	if (r != 0)
		token = "";
	return true;
}

/**
  * @brief A1 notation
  * @param column 0-> A
  */
static std::string columnFromNumber
(
	int column
)
{
	std::string columnString;
	int columnNumber = column + 1;
	while (columnNumber > 0)
    {
        int currentLetterNumber = (columnNumber - 1) % 26;
        char currentLetter = (char)(currentLetterNumber + 65);
        columnString = currentLetter + columnString;
        columnNumber = (columnNumber - (currentLetterNumber + 1)) / 26;
    }
    return columnString;
}

/**
  * @brief return A1 notation. column and row are zero based index 
  * @param column 0->A
  * @param row 0->1
  */
std::string GoogleSheets::A1
(
	const std::string &sheet,
	int col, 
	int row
)
{
	std::stringstream ss;
	if (!sheet.empty())
		ss << sheet << "!";
	ss << columnFromNumber(col) << row + 1;
	return ss.str(); 
}

std::string GoogleSheets::A1(
	const std::string &sheet,
	int col0, 
	int row0,
	int col1, 
	int row1
)
{
	std::stringstream ss;
	if (!sheet.empty())
		ss << sheet << "!";
	ss << columnFromNumber(col0) << row0 + 1 << ":" << columnFromNumber(col1) << row1 + 1; 
	return ss.str(); 
}


/**
  * @brief get values in range
  */
bool GoogleSheets::get
(
	const std::string &sheet,
	int col, 
	int row,
	ValueRange &retval
)
{
	return get(A1(sheet, col, row), retval);
}

/**
  * @brief check is Google sheets request a new token bearer
  * Error response e.g.
  * {
  * "error": {
  *   "code": 401,
  *   "message": "Request had invalid authentication credentials. Expected OAuth 2 access token, login cookie or other valid authentication credential. See https://developers.google.com/identity/sign-in/web/devconsole-project.",
  *   "status": "UNAUTHENTICATED"
  * }
}

  */
int GoogleSheets::checkJSONErrorCode
(
	const std::string &response
)
{
	if (!response.empty())
	{
		// parse response
		Json::Reader reader;
		Json::Value v;
		if (!reader.parse(response, v))
			return 200;
		if (v.isMember("error"))
		{
			Json::Value e = v["error"];
			if (e["code"].isInt())
				return e["error"].asInt();
		}
	}
	return 200;
}

/**
  * @brief GET spreadsheet range values. If token expires, regenerate token bearer and try again
  */
bool GoogleSheets::token_get(
	const std::string &url,
	ValueRange &retval
)
{
	std::string response;
	int r = curl_get(token, url, response);
	if (checkJSONErrorCode(response) != 200)
	{
		genToken();
		r = curl_get(token, url, response);
	}
	bool ok = (r == 200);
	if (ok)
		retval.parseJSON(response);
	else
		retval.clear();
	return ok;
}

/**
  * @brief PUT spreadsheet range values. If token expires, regenerate token bearer and try again
  */
bool GoogleSheets::token_put(
	const std::string &url,
	const ValueRange &values
)
{
	std::string json = values.toJSON();
	std::string response;
	int r = curl_put(token, url, json, response);
	if (r != 200)
	{
		genToken();
		r = curl_put(token, url, json, response);
	}
	return (r == 200);
}

/**
  * @brief POST spreadsheet range values. If token expires, regenerate token bearer and try again
  */
bool GoogleSheets::token_post
(
	const std::string &url,
	const ValueRange &values
)
{
	std::string json = values.toJSON();
	std::string response;
	int r = curl_post(token, url, json, response);
	
	// std::cerr << json << std::endl;
	
	if (r != 200)
	{
		genToken();
		r = curl_put(token, url, json, response);
	}
	return r == 200;
}


/**
 * @brief get values in a range
 * @param range Sheet1!A1:D5
 */
bool GoogleSheets::get
(
	const std::string &range, 
	ValueRange &response
)
{
	return token_get(API_SHEET + sheet_id + "/values/" + range, response);
}

/**
 * @brief Replace values in a range 
 * @param values value range
 */
bool GoogleSheets::put
(
	const ValueRange &values
)
{
	return token_put(API_SHEET + sheet_id + "/values/" + values.range + "?valueInputOption=USER_ENTERED", values);
}

/**
 * @brief Append values to a range
 * @param values value range
 */
bool GoogleSheets::append
(
	const ValueRange &values
)
{
	std::string u(API_SHEET + sheet_id + "/values/" + values.range + ":append?valueInputOption=USER_ENTERED");
	return token_post(u, values);
}
