#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>
#include <stdint.h>

#define PROGRAM_NAME             "pkt2receiver"
#define PROGRAM_DESCRIPTION      "PKT2 receiver"

/**
 * Parse command line options
 * @param argc
 * @param argv
 * @see pktreceiver.cpp
 */
class Config
{
private:
    int lastError;
    /**
    * @brief Parse command line
    * @return 0- success
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

    int retries;            ///< default 0
    int retry_delay;        ///< default 60 seconds

    bool daemonize;
    bool stop_request;
    std::string in_url;
    std::string out_url;
};


#endif
