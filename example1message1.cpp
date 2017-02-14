#include <string>
#include <iostream>
#include <signal.h>

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/text_format.h>
#include "example/example1.pb.h"

bool writeTo(
    const google::protobuf::MessageLite& message,
    google::protobuf::io::ZeroCopyOutputStream* rawOutput)
{
	// We create a new coded stream for each message.  Don't worry, this is fast.
	google::protobuf::io::CodedOutputStream output(rawOutput);

	const int size = message.ByteSize();

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

int main(int args, char **argv)
{
	example1::TemperaturePkt m;
	std::ostream *ostrm = &std::cout;

	google::protobuf::io::OstreamOutputStream strm(ostrm);

	time_t t = time(NULL);
	srand(t);
	double c = 20.0 + ((10. * rand()) / RAND_MAX);
	m.set_device(rand());
	t = time(NULL);
	m.set_time(t);
	m.set_degrees_c(c);
	writeTo(m, &strm);
}
