#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>

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

/**
 *	remove all non numeric characters from the phone number
 *	TODO use libphonenumber
 */
std::string E164ToString(const std::string &value);

/**
 *	Format phone number
 *	TODO use libphonenumber
 */
std::string String2E164(const std::string &value);

/**
 *	remove all non numeric characters from the phone number
 *	TODO use libphonenumber
 */
uint64_t E164ToLong(const std::string &value);

/**
 *	Format phone number
 *	TODO use libphonenumber
 */

std::string Long2E164(const uint64_t value);


std::string uint64ToString(const uint64_t value);

std::string doubleToString(const double value);
