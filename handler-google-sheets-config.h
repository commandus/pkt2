#ifndef HANDLER_LINE_CONFIG_H
#define HANDLER_LINE_CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "handler-google-sheets"
#define PROGRAM_DESCRIPTION      "Google sheet printer"

/**
  * @brief google-sheets-writer command line options
  * @see google-sheets-writer.cpp
  */
class Config
{
private:
    int lastError;
    /**
    * Parse command line
    * Return 0- success
    **/
    int parseCmd
    (
            int argc,
            char* argv[]
    );
public:
    Config(int argc, char* argv[]);
    int error();

    std::string message_url; ///< nano message URL
    int retries;             ///< default 1
    int retry_delay;         ///<

    // proto
    std::string proto_path;	///< proto files directory path
    int mode;		        ///< default 0
    int buffer_size;        ///< default 2048

    // OAuth, Google sheets
    std::string client;
    std::string secret;
    std::string sheet;

    bool daemonize;
    int max_fd;				///< 0- use default max file descriptor count per process
    bool stop_request;
    int verbosity;          ///< default 0
};


#endif
