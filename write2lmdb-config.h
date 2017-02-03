#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "write2lmdb"
#define PROGRAM_DESCRIPTION      "PKT2 LMDB witer"

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

    std::string message_url; ///< nano message URL
    int retries;             ///< default 1
    int retry_delay;         ///<

    std::string path;        ///< path, default "lmdb"
    int mode;                ///< default 0664
    int flags;               ///< default 0

    bool daemonize;
    bool stop_request;
};


#endif
