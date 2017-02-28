#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <sys/socket.h>

// trim from start
std::string &ltrim(std::string &s);

// trim from end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

// read file
std::string file2string(const std::string &filename);

// read file
std::string file2string(const char *filename);

// write file
bool string2file(const std::string &filename, const std::string &value);

// write file
bool string2file(const char *filename, const std::string &value);

/**
 * Split string
 * See http://stackoverflow.com/questions/236129/split-a-string-in-c
 */
std::vector<std::string> split(const std::string &s, char delim);

template <typename T> std::string toString(const T value)
{
        std::stringstream idss;
        idss << value;
        return idss.str();
}

std::string doubleToString(const double value);

void *get_in_addr(struct sockaddr *sa);

std::string sockaddrToString(struct sockaddr_storage *value);

std::string timeToString(time_t value);

std::string spaces(char ch, int count);
