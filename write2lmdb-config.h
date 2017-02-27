#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>

#define PROGRAM_NAME             "write2lmdb"
#define PROGRAM_DESCRIPTION      "PKT2 LMDB writer"

/**
  *      \see write2lmdb.cpp
  *      \see lmdbwritee.cpp
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

    std::string path;        ///< path, default "lmdb"
    int mode;                ///< default 0664
    int flags;               ///< default 0

    bool daemonize;
    bool stop_request;
};


#endif
