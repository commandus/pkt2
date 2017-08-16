#include <nanomsg/nn.h>

#include "freceiver-config.h"
#include "utilstring.h"

void readData
(
	std::ostream &ostrm,
	int cmd,
	int packet_size,
	std::istream *istrm
);

int file_receiever(Config *config);
int stop(Config *config);
int reload(Config *config);
