#ifndef CONFIG_H
#define CONFIG_H     1

#include <string>

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

        std::string intface;
        uint32_t port;
};


#endif