#include <string>
#include <iostream>
#include <signal.h>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#else
#endif

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>
#include "utilprotobuf.h"
#include "example/example1.pb.h"
#include "errorcodes.h"

#define MAX_COUNT	1000000

bool cont;

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		std::cerr << MSG_INTERRUPTED;
		cont = false;
		break;
#if defined(_WIN32) || defined(_WIN64)
#else
	case SIGHUP:
		std::cerr << MSG_RELOAD_CONFIG_REQUEST << " nothing to do";
		break;
#endif
	default:
		std::cerr << MSG_SIGNAL << signal;
	}
}

#if defined(_WIN32) || defined(_WIN64)
#else
void setSignalHandler(int signal)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	sigaction(signal, &action, NULL);
}
#endif

int main(int args, char **argv)
{
    // Signal handler
#if defined(_WIN32) || defined(_WIN64)
#else
    setSignalHandler(SIGINT);
	setSignalHandler(SIGHUP);
#endif

 	example1::TemperaturePkt m;
	std::ostream *ostrm = &std::cout;

	google::protobuf::io::OstreamOutputStream strm(ostrm);

	time_t t = time(NULL);
	srand(t);
	int count = 0;
	cont = true;

	MessageTypeNAddress messageTypeNAddress(m.GetDescriptor()->full_name());	// "example1.TemperaturePkt"

	int i = 0;
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
		i++;
		if (i >= MAX_COUNT)
			break;
	}
}
