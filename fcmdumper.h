#include "pkt2dumpfcm-config.h"

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
