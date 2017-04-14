#include "handlerline-config.h"

// output modes
#define MODE_JSON     0
#define MODE_JSON_RAW 1
#define MODE_CSV      2
#define MODE_TAB      3
#define MODE_SQL      4
#define MODE_SQL2     5
#define MODE_OPTIONS  6

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
