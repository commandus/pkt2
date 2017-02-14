#include <string>
#include <iostream>
#include <signal.h>

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>
#include "example/example1.pb.h"

bool writeDelimitedTo(
    const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput)
{
	// We create a new coded stream for each message.  Don't worry, this is fast.
	google::protobuf::io::CodedOutputStream output(rawOutput);

	// Write the size.
	const int size = message.ByteSize();
	output.WriteVarint32(size);

	uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
	if (buffer != NULL)
	{
		// Optimization:  The message fits in one buffer, so use the faster
		// direct-to-array serialization path.
		message.SerializeWithCachedSizesToArray(buffer);
	}
	else
	{
		// Slightly-slower path when the message is multiple buffers.
		message.SerializeWithCachedSizes(&output);
		if (output.HadError()) return false;
	}
	return true;
}

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
	while (cont)
	{
		for (int d = 0; d < 3; d++)
		{
			double c = 20.0 + ((10. * rand()) / RAND_MAX);
			m.set_device(d);
			t = time(NULL);
			m.set_time(t);
			m.set_degrees_c(c);
			writeDelimitedTo(m, &strm);
			count++;
		}
	}
}
