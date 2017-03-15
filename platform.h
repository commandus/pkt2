#define VERSION "0.1"


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
   	google::SetLogDestination(google::INFO, PROGRAM_NAME); \
