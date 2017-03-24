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
#include <iomanip>

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

std::string replace(const std::string &str, const std::string &from, const std::string &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
    	return str;
	std::string ret(str);
	ret.replace(start_pos, from.length(), to);
	return ret;
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

// http://stackoverflow.com/questions/673240/how-do-i-print-an-unsigned-char-as-hex-in-c-using-ostream
struct HexCharStruct
{
        unsigned char c;
        HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs)
{
        return (o << std::setfill('0') << std::setw(2) << std::hex << (int) hs.c);
}

inline HexCharStruct hex(unsigned char c)
{
        return HexCharStruct(c);
}

void bufferPrintHex(std::ostream &sout, void* value, size_t size)
{
        if (value == NULL)
                return;
        unsigned char *p = (unsigned char*) value;
        for (size_t i = 0; i < size; i++)
        {
                sout << hex(*p);
                p++;
        }
}

std::string hexString(void *buffer, size_t size)
{
        std::stringstream r;
        bufferPrintHex(r, buffer, size);
        return r.str();
}

