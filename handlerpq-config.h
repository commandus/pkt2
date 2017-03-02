#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <stdint.h>

#define PROGRAM_NAME             "handlerpq"
#define PROGRAM_DESCRIPTION      "PostgreSQLreceiver"

/**
  * @brief  
  * @see
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

    uint32_t verbosity;
    size_t buffer_size;

    bool daemonize;
    bool stop_request;
    std::string message_url;
};


#endif
