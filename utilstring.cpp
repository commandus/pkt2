#include "utilstring.h"

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <ctime>
#include <limits>
#include <locale>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <arpa/inet.h>

/// http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from start
std::string &ltrim(std::string &s) 
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
std::string &rtrim(std::string &s) 
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
std::string &trim(std::string &s) 
{
	return ltrim(rtrim(s));
}

std::string file2string(const std::string &filename)
{
	return file2string(filename.c_str());
}

std::string file2string(const char *filename)
{
	if (!filename)
		return "";
	std::ifstream t(filename);
	return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

bool string2file(const std::string &filename, const std::string &value)
{
	return string2file(filename.c_str(), value);
}

bool string2file(const char *filename, const std::string &value)
{
	if (!filename)
		return "";
	FILE* f = fopen(filename, "w");
	if (!f)
		return false;
	fwrite(value.c_str(), value.size(), 1, f);
	fclose(f);
	return true;

}

/**
 * Split
 * http://stackoverflow.com/questions/236129/split-a-string-in-c
 */
void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

inline bool predicate_num(char c)
{
	return !isdigit(c);
};

/**
 *	remove all non numeric characters from the phone number
 *	TODO use libphonenumber
 */
std::string E164ToString(const std::string &value)
{
	std::string r(value);
	r.erase(std::remove_if(r.begin(), r.end(), predicate_num), r.end());
	return r;
}

/**
 *	Format phone number
 *	TODO use libphonenumber
 */

std::string String2E164(const std::string &value)
{
	if (value.size() == 11)
	{
		std::ostringstream os;
		os << "+" << value.substr(0,1) << "(" << value.substr(1,3) <<  ")" << value.substr(4,3) << "-" << value.substr(7,4);
		return os.str();
	}
	else
		return "";
}

/**
 *	remove all non numeric characters from the phone number
 *	TODO use libphonenumber
 */
uint64_t E164ToLong(const std::string &value)
{
	if (&value == NULL)
		return 0;

	std::string v(value);
	v.erase(std::remove_if(v.begin(), v.end(), predicate_num), v.end());
	return strtoul(v.c_str(), NULL, 0);
}

/**
 *	Format phone number
 *	TODO use libphonenumber
 */

std::string Long2E164(const uint64_t value)
{
	std::ostringstream os;
	os << value;
	std::string v(os.str());
	// TODO make valid for other E164 codes
	if (v.size() == 11)
	{
		std::ostringstream os;
		os << "+" << v.substr(0,1) << "(" << v.substr(1,3) <<  ")" << v.substr(4,3) << "-" << v.substr(7,4);
		return os.str();
	}
	else
		return "";
}

std::string uint64ToString(const uint64_t value)
{
	std::stringstream idss;
	idss << value;
	return idss.str();
}

std::string doubleToString(const double value)
{
	std::stringstream idss;
	idss.precision(std::numeric_limits<double>::digits10 + 1);
	idss << std::fixed << value;
	return idss.str();
}

void *get_in_addr
(
    struct sockaddr *sa
)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr); 
	return &(((struct sockaddr_in6 *)sa)->sin6_addr); 
}

std::string sockaddrToString
(
	struct sockaddr_storage *value
)
{
	char s[INET6_ADDRSTRLEN]; // an empty string 
	inet_ntop(value->ss_family, get_in_addr((struct sockaddr *) value), s, sizeof(s)); 
	return s;
}

std::string timeToString(time_t value)
{
	if (!value)
		value = std::time(NULL);
	std::tm *ptm = std::localtime(&value);
	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%c", ptm);
	return std::string(buffer);
}

std::string spaces(char ch, int count)
{
    std::stringstream ss;
    for (int i = 0; i < count; i++) {
        ss << ch;
    }
    return ss.str();
}
