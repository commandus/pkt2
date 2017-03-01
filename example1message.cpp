#include <string>
#include <iostream>
#include <signal.h>

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>
#include "utilprotobuf.h"
#include "example/example1.pb.h"

bool cont;

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		cont = false;
		std::cerr << "Interrupted" << std::endl;
		break;
	default:
		break;
	}
}

void setSignalHandler(int signal)
{
        struct sigaction action;
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_handler = &signalHandler;
        sigaction(signal, &action, NULL);
}

int main(int args, char **argv)
{
    // Signal handler
    setSignalHandler(SIGINT);

	example1::TemperaturePkt m;
	std::ostream *ostrm = &std::cout;

	google::protobuf::io::OstreamOutputStream strm(ostrm);

	time_t t = time(NULL);
	srand(t);
	int count = 0;
	cont = true;

	MessageTypeNAddress messageTypeNAddress("example1.TemperaturePkt");

	while (cont)
	{
		for (int d = 0; d < 3; d++)
		{
			double c = 20.0 + ((10. * rand()) / RAND_MAX);
			m.set_device(d);
			t = time(NULL);
			m.set_time(t);
			m.set_degrees_c(c);
			writeDelimitedMessage(&messageTypeNAddress, m, &strm);
			count++;
		}
	}
}
