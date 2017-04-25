#include "handlerline-config.h"

// output modes
#define MODE_JSON     0
#define MODE_CSV      1
#define MODE_TAB      2
#define MODE_SQL      3
#define MODE_SQL2     4

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
