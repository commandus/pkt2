#include "utilstring.h"

#include <stdlib.h>
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
std::string &pkt2utilstring::ltrim(std::string &s) 
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
std::string &pkt2utilstring::rtrim(std::string &s) 
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
std::string &pkt2utilstring::trim(std::string &s) 
{
	return ltrim(rtrim(s));
}

std::string pkt2utilstring::replace(const std::string &str, const std::string &from, const std::string &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
    	return str;
	std::string ret(str);
	ret.replace(start_pos, from.length(), to);
	return ret;
}

std::string pkt2utilstring::file2string(std::istream &strm)
{
	if (!strm)
		return "";
	return std::string((std::istreambuf_iterator<char>(strm)), std::istreambuf_iterator<char>());
}

std::string pkt2utilstring::file2string(const char *filename)
{
	if (!filename)
		return "";
	std::ifstream t(filename);
	return file2string(t);
}

std::string pkt2utilstring::file2string(const std::string &filename)
{
	return file2string(filename.c_str());
}

bool pkt2utilstring::string2file(const std::string &filename, const std::string &value)
{
	return string2file(filename.c_str(), value);
}

bool pkt2utilstring::string2file(const char *filename, const std::string &value)
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
static void split3(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

std::vector<std::string> pkt2utilstring::split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split3(s, delim, elems);
    return elems;
}

inline bool predicate_num(char c)
{
	return !isdigit(c);
};

std::string pkt2utilstring::doubleToString(const double value)
{
	std::stringstream idss;
	idss.precision(std::numeric_limits<double>::digits10 + 1);
	idss << std::fixed << value;
	return idss.str();
}

void *pkt2utilstring::get_in_addr
(
    struct sockaddr *sa
)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr); 
	return &(((struct sockaddr_in6 *)sa)->sin6_addr); 
}

std::string pkt2utilstring::sockaddrToString
(
	struct sockaddr_storage *value
)
{
	char s[INET6_ADDRSTRLEN]; // an empty string 
	inet_ntop(value->ss_family, pkt2utilstring::get_in_addr((struct sockaddr *) value), s, sizeof(s)); 
	return s;
}

std::string pkt2utilstring::timeToString(time_t value)
{
	if (!value)
		value = std::time(NULL);
	std::tm *ptm = std::localtime(&value);
	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%c", ptm);
	return std::string(buffer);
}

std::string pkt2utilstring::spaces(char ch, int count)
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

static void bufferPrintHex(std::ostream &sout, const void* value, size_t size)
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

std::string pkt2utilstring::hexString(const void *buffer, size_t size)
{
	std::stringstream r;
	bufferPrintHex(r, buffer, size);
	return r.str();
}

/**
 * Return hex string
 * @param data
 * @return
 */
std::string pkt2utilstring::hexString(const std::string &data)
{
	return pkt2utilstring::hexString((void *) data.c_str(), data.size());
}

static std::string readHex(std::istream &s)
{
	std::stringstream r;
	s >> std::noskipws;
	char c[3] = {0, 0, 0};
	while (s >> c[0])
	{
		if (!(s >> c[1]))
			break;
		unsigned char x = (unsigned char) strtol(c, NULL, 16);
		r << x;
	}
	return r.str();
}

std::string pkt2utilstring::hex2string(const std::string &hex)
{
	std::stringstream ss(hex);
    return readHex(ss);
}

/**
 * @brief Return binary data string
 * @param hex hex string
 * @return binary data string
 */
std::string pkt2utilstring::stringFromHex(const std::string &hex)
{
	std::string r(hex.length() / 2, '\0');
	std::stringstream ss(hex);
	ss >> std::noskipws;
	char c[3] = {0, 0, 0};
	int i = 0;
	while (ss >> c[0]) {
		if (!(ss >> c[1]))
			break;
		unsigned char x = (unsigned char) strtol(c, NULL, 16);
		r[i] = x;
		i++;
	}
	return r;
}

std::string pkt2utilstring::arg2String(
	int argc, 
	char *argv[]
)
{
	std::stringstream ss;
	for (int i = 0; i < argc; i ++)
	{
		ss << argv[i] << " ";
	}
	return ss.str();
}
