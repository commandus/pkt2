/*
 * oauth2.h
 */

#ifndef OAUTH2_H_
#define OAUTH2_H_

#include <string>

std::string loadGoogleToken
(
	const std::string &service_account,
	const std::string &subject_email,
	const std::string &pemkey,
	const std::string &scope,
	const std::string &audience,
	int expires
);

std::string curl_post
(
		const std::string &url,
		const std::string &data
);

#endif /* OAUTH2_H_ */
