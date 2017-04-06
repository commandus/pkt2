/*
 * oauth2.h
 */

#ifndef OAUTH2_H_
#define OAUTH2_H_

#include <string>

/**
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
);

class OAuth2 {
public:
	OAuth2();
	virtual ~OAuth2();
	std::string getToken
	(
		const std::string &consumer_key, 
		const std::string &consumer_secret,
		const std::string &client,
		const std::string &scope
	);
	
	
};

#endif /* OAUTH2_H_ */
