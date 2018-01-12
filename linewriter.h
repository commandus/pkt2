#include "handlerline-config.h"

#include "messageformat.h"

/**
  * Return:  0- success
  *          1- can not listen port
  *          2- invalid nano socket URL
  *          3- buffer allocation error
  *          4- send error, re-open 
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
