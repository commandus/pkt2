#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "message2gateway"
#define PROGRAM_DESCRIPTION      "protobuf message injector. Read protobuf message(s) from stdin or file. Each message must be delimited."

/**
  *     @brief message2gateway configuration
  *     @see message2gateway.cpp
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

    std::string message_in_url;
    std::string message_out_url;
    std::string file_name;
    std::string proto_path;	///< proto files directory path

    bool daemonize;
    bool stop_request;
    size_t buffer_size;
};


#endif
