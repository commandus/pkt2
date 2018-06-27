#include <string>

/**
* @brief Push notification to device
*/
int push2instance
(
	std::string &retval,
	const std::string &url,
	const std::string &serverkey,
	const std::string &client_token,
	const std::string &name,
	const int tz,
	const std::string &data
);
