/*
 * oauth2.cpp
 *
 */
#include <iostream>
#include <algorithm>
#include <json/json.h>
#include <glog/logging.h>

#include <openssl/conf.h>
#include <openssl/x509v3.h>

#include "sslhelper.h"
#include "utilstring.h"
#include "oauth2.h"
#include <liboauthcpp/liboauthcpp.h>

std::string getUserString(std::string prompt) {
    std::cout << prompt << " ";

    std::string res;
    std::cin >> res;
    std::cout << std::endl;
    return res;
}

OAuth2::OAuth2() {
	// TODO Auto-generated constructor stub

}

OAuth2::~OAuth2() {
	// TODO Auto-generated destructor stub
}

std::string OAuth2::getToken
(
	const std::string &client,
	const std::string &scope,
	const std::string &consumer_key, 
	const std::string &consumer_secret
) 
{
	https://accounts.google.com/o/oauth2/auth?client_id=995029341446-8lsujer4ttgomr0lllt4vpeflbjpeoof.apps.googleusercontent.com&oauth_consumer_key=995029341446-8lsujer4ttgomr0lllt4vpeflbjpeoof.apps.googleusercontent.com&oauth_nonce=14913771112cff0a79&oauth_signature=TcY7N6Y4K%2BYfqPFOoKA4vsf4M40%3D&oauth_signature_method=HMAC-SHA1&oauth_timestamp=1491377111&oauth_version=1.0&redirect_uri=https%3A%2F%2Flocalhost&response_type=code&scope=https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fspreadsheets

	std::string request_token_url = "https://accounts.google.com/o/oauth2/auth";
	std::string authorize_url = "https://api.twitter.com/oauth/authorize";
	std::string access_token_url = "https://api.twitter.com/oauth/access_token";

    OAuth::Consumer consumer(consumer_key, consumer_secret);
    OAuth::Client oauth(&consumer);
    std::string base_request_token_url = request_token_url + "?response_type=code&redirect_uri=https%3A%2F%2Flocalhost&client_id=" 
    	+ client + "&scope=" + scope;
    std::string oAuthQueryString = oauth.getURLQueryString(OAuth::Http::Get, base_request_token_url);

    std::cout << "Enter the following in your browser to get the request token: " << std::endl;
    std::cout << request_token_url << "?" << oAuthQueryString << std::endl;
    std::cout << std::endl;

    // Extract the token and token_secret from the response
    std::string request_token_resp = getUserString("Enter the response:");
    // This time we pass the response directly and have the library do the
    // parsing (see next extractToken call for alternative)
    OAuth::Token request_token = OAuth::Token::extract( request_token_resp );

    // Get access token and secret from OAuth object
    std::cout << "Request Token:" << std::endl;
    std::cout << "    - oauth_token        = " << request_token.key() << std::endl;
    std::cout << "    - oauth_token_secret = " << request_token.secret() << std::endl;
    std::cout << std::endl;

    // Step 2: Redirect to the provider. Since this is a CLI script we
    // do not redirect. In a web application you would redirect the
    // user to the URL below.
    std::cout << "Go to the following link in your browser to authorize this application on a user's account:" << std::endl;
    std::cout << authorize_url << "?oauth_token=" << request_token.key() << std::endl;

    // After the user has granted access to you, the consumer, the
    // provider will redirect you to whatever URL you have told them
    // to redirect to. You can usually define this in the
    // oauth_callback argument as well.
    std::string pin = getUserString("What is the PIN?");
    request_token.setPin(pin);

    // Step 3: Once the consumer has redirected the user back to the
    // oauth_callback URL you can request the access token the user
    // has approved. You use the request token to sign this
    // request. After this is done you throw away the request token
    // and use the access token returned. You should store the oauth
    // token and token secret somewhere safe, like a database, for
    // future use.
    oauth = OAuth::Client(&consumer, &request_token);
    // Note that we explicitly specify an empty body here (it's a GET) so we can
    // also specify to include the oauth_verifier parameter
    oAuthQueryString = oauth.getURLQueryString( OAuth::Http::Get, access_token_url, std::string( "" ), true );
    std::cout << "Enter the following in your browser to get the final access token & secret: " << std::endl;
    std::cout << access_token_url << "?" << oAuthQueryString;
    std::cout << std::endl;

    // Once they've come back from the browser, extract the token and token_secret from the response
    std::string access_token_resp = getUserString("Enter the response:");
    // On this extractToken, we do the parsing ourselves (via the library) so we
    // can extract additional keys that are sent back, in the case of twitter,
    // the screen_name
    OAuth::KeyValuePairs access_token_resp_data = OAuth::ParseKeyValuePairs(access_token_resp);
    OAuth::Token access_token = OAuth::Token::extract( access_token_resp_data );

    std::cout << "Access token:" << std::endl;
    std::cout << "    - oauth_token        = " << access_token.key() << std::endl;
    std::cout << "    - oauth_token_secret = " << access_token.secret() << std::endl;
    std::cout << std::endl;
    std::cout << "You may now access protected resources using the access tokens above." << std::endl;
    std::cout << std::endl;

    std::pair<OAuth::KeyValuePairs::iterator, OAuth::KeyValuePairs::iterator> screen_name_its = access_token_resp_data.equal_range("screen_name");
    for(OAuth::KeyValuePairs::iterator it = screen_name_its.first; it != screen_name_its.second; it++)
        std::cout << "Also extracted screen name from access token response: " << it->second << std::endl;

    // E.g., to use the access token, you'd create a new OAuth using
    // it, discarding the request_token:
    // oauth = OAuth::Client(&consumer, &access_token);
}

std::string jws_sign
(
	const std::string &data, 
	const std::string &p12filename,
	const char *password
)
{
    SHA256_CTX mctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Init(&mctx);
    SHA256_Update(&mctx, data.c_str(), data.size());
    SHA256_Final(hash, &mctx);

	unsigned char *sig;
	size_t slen;
	
    EVP_PKEY *key = p12ReadPKey(p12filename, password);
    EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new(key, NULL);
LOG(INFO) << "jws_sign 1";    
    if (!kctx) 
    	goto err;
LOG(INFO) << "jws_sign 2";
    if (EVP_PKEY_sign_init(kctx) <= 0) 
    	goto err;
LOG(INFO) << "jws_sign 3";
    if (EVP_PKEY_CTX_set_rsa_padding(kctx, RSA_PKCS1_PADDING) <= 0) 
    	goto err;
LOG(INFO) << "jws_sign 4";
    if (EVP_PKEY_CTX_set_signature_md(kctx, EVP_sha256()) <= 0) 
    	goto err;
LOG(INFO) << "jws_sign 5";
    // Determine buffer length
    slen = 0;
    if (EVP_PKEY_sign(kctx, NULL, &slen, hash, SHA256_DIGEST_LENGTH) <= 0) 
    	goto err;
LOG(INFO) << "jws_sign 6";
    sig = (unsigned char *) OPENSSL_malloc(slen);
    if (!sig) 
    	goto err;
LOG(INFO) << "jws_sign 7";
    if (EVP_PKEY_sign(kctx, sig, &slen, hash, SHA256_DIGEST_LENGTH) <= 0) 
    	goto err;
LOG(INFO) << "jws_sign 8";
    return Base64Encode(sig, (unsigned int) slen);

err:
    // Clean up
    EVP_cleanup();
    return "";
}

/**
  * @param pem_pkey
  * @param scope https://www.googleapis.com/auth/spreadsheets https://www.googleapis.com/auth/plus.me
  *	@param service_account accounts.google.com, https://accounts.google.com. 
  * @param audience app's client IDs 
  * @param expires 3600
  */
std::string getJWT
(
	const std::string &p12filename,
	const std::string &password,
	const std::string &scope,
	const std::string &audience,
	const std::string &service_account,
	int expires
)
{
	Json::Value jwt_header;
    Json::Value jwt_claim_set;
    std::string jwt_b64;
    std::time_t t = std::time(NULL);
    Json::FastWriter jfw;
    Json::StyledWriter jsw;

    // Create jwt header
    jwt_header["alg"] = "RS256";
    jwt_header["typ"] = "JWT";

    // jwt claim set
    jwt_claim_set["iss"] = service_account;
    jwt_claim_set["scope"] = scope;
    // intended target of the assertion for an access token
    jwt_claim_set["aud"] = audience;
    jwt_claim_set["iat"] = std::to_string(t);
    jwt_claim_set["exp"] = std::to_string(t + expires);

    std::cout << jsw.write(jwt_header) << "." << jsw.write(jwt_claim_set);

    // create http POST request body
    // for header
    std::string json_buffer;
    std::string json_buffer1;
    json_buffer = jfw.write(jwt_header);
    json_buffer = json_buffer.substr(0, json_buffer.size() - 1);
    json_buffer = Base64Encode(reinterpret_cast<const unsigned char*>(json_buffer.c_str()), json_buffer.length()); 
    json_buffer1.clear();
    std::remove_copy(json_buffer.begin(), json_buffer.end(), std::back_inserter(json_buffer1), '=');
    jwt_b64 = json_buffer1;
    jwt_b64 += ".";
LOG(INFO) << "header: " << json_buffer1;
    // for claim set
    json_buffer = jfw.write(jwt_claim_set);
    json_buffer = json_buffer.substr(0, json_buffer.size() - 1);
    json_buffer = Base64Encode(reinterpret_cast<const unsigned char*>(json_buffer.c_str()), json_buffer.length());
    json_buffer1.clear();
    std::remove_copy(json_buffer.begin(), json_buffer.end(), std::back_inserter(json_buffer1), '=');
    jwt_b64 += json_buffer1;
LOG(INFO) << "claim set: " << json_buffer1;
    // for signature
    std::string jwt_signature = jws_sign(jwt_b64, p12filename, (password.empty() ? NULL : password.c_str()));
    
    jwt_b64 += ".";
    json_buffer1.clear();
    std::remove_copy(jwt_signature.begin(), jwt_signature.end(), std::back_inserter(json_buffer1), '=');
LOG(INFO) << "signature: " << json_buffer1;
    jwt_b64 += json_buffer1;
    
	// for test purpose calling with curl
    std::cout << "curl -d 'grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" << jwt_b64
    	<< "' https://www.googleapis.com/oauth2/v4/token";

LOG(INFO) << "jwt_b64: " <<  jwt_b64;
    return jwt_b64;
}
