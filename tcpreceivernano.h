#include <nanomsg/nn.h>

#include "tcpreceiver-config.h"
#include "utilstring.h"

int tcp_receiever_nano(Config *config);
int stop(Config *config);
int reload(Config *config);
