#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "message2gateway"
#define PROGRAM_DESCRIPTION      "protobuf message injector. Read protobuf message(s) from stdin or file. Each message must be delimited."

/**
  *      \see
  */
class Config
{
private:
    int lastError;
    /**
    * Parse command line
    * Return 0- success
    *        1- show help and exit, or command syntax error
    *        2- output file does not exists or can not open to write
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

    std::string message_url;
    std::string file_name;

    bool daemonize;
    bool stop_request;
};


#endif
