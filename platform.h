#define VERSION "0.1"

#ifdef _WIN32
#include "win.h"
#else
#include <time.h>
#endif

#ifdef __cplusplus
#define CALLC extern "C" 
#else
#define CALLC
#endif

#define INIT_LOGGING(PROGRAM_NAME) \
	google::InitGoogleLogging(argv[0]); \
	google::InstallFailureSignalHandler(); \
	FLAGS_logtostderr = !config->verbosity; \
	FLAGS_minloglevel = 2 - config->verbosity; \
   	google::SetLogDestination(google::INFO, PROGRAM_NAME);

#define ENDIAN_NETWORK pkt2::Endian::ENDIAN_BIG_ENDIAN
#define ENDIAN_HOST pkt2::ENDIAN_LITTLE_ENDIAN
#define ENDIAN_NEED_SWAP(v) ((v != pkt2::ENDIAN_NO_MATTER) && (v != ENDIAN_HOST))

#ifdef _WIN32
#define SLEEP(seconds) \
    Sleep(seconds *1000);

#define SEND_FLUSH \
    Sleep(100);
#define WAIT_CONNECTION \
	Sleep(1000); // wait for connections
#else
#define SLEEP(seconds) \
    sleep(seconds);
#define SEND_FLUSH(milliseconds) \
{ \
	sleep(0); \
    struct timespec ts; \
    ts.tv_sec = milliseconds / 1000;  \
    ts.tv_nsec = (milliseconds % 1000) * 1000000; \
    nanosleep (&ts, NULL); \
}

#define WAIT_CONNECTION(timeout) \
	sleep(timeout); // wait for connections

#endif
