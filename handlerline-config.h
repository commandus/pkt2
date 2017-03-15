#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "handlerline"
#define PROGRAM_DESCRIPTION      "PKT2 stdout printer"

/**
  *      \see handlerline.cpp
  *      \see handlerline.cpp
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
    int mode;		        ///< default 0- JSON
    int buffer_size;        ///< default 2048

    bool daemonize;
    bool stop_request;
    int verbosity;          ///< default 0
};


#endif
