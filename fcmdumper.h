#include "pkt2dumpfcm-config.h"

/**
 * @brief Send notification to the mobile device
 * @param hex hex string
 * @param config Configuration
 */
int sendNotifications
(
	const std::string hex, 
	Config *config
);

int run
(
		Config *config
);

/**
  * Return 0- success
  *        1- config is not initialized yet
  */
int stop
(
		Config *config
);


int reload(Config *config);
