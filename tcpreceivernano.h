#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "tcpreceiver-config.h"
#include "utilstring.h"

int tcp_receiever_nano(Config *config);
int stop(Config *config);
