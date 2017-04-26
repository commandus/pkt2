#ifndef TCP_EMITTER_CONFIG_H
#define TCP_EMITTER_CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "messageemitter"
#define PROGRAM_DESCRIPTION      "Protobuf message injector. Read message from stdin, TCP socket or file line by line"

#define DEF_PORT                 50052
#define DEF_ADDRESS              "0.0.0.0"

/**
  *     @brief messageemitter configuration
  *     @see messageemitter.cpp
  */
class Config
{
private:
    int lastError;
    /**
    * @brief Parse command line
    * @return 0- success
    **/
    int parseCmd
    (
            int argc,
            char* argv[]
    );
public:
    Config(int argc, char* argv[]);
    int error();

    int verbosity;

    int retries;            ///< default 0
    int retry_delay;        ///< default 60 seconds

    std::string message_out_url;
    std::string file_name;
    std::string proto_path;	///< proto files directory path

    std::string intface;    ///< default 0.0.0.0
    int port;				///< default 50052

    bool daemonize;
    int stop_request;		///< 0- process, 1- stop request, 2- reload request
};


#endif

