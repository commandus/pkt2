#include "pkt2dumppq-config.h"

// output modes
#define MODE_SQL      3
#define MODE_SQL2     4

/**
  */
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
