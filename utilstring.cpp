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

std::string file2string(std::istream &strm)
{
	if (!strm)
		return "";
	return std::string((std::istreambuf_iterator<char>(strm)), std::istreambuf_iterator<char>());
}

std::string file2string(const char *filename)
{
	if (!filename)
		return "";
	std::ifstream t(filename);
	return file2string(t);
}

std::string file2string(const std::string &filename)
{
	return file2string(filename.c_str());
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

/**
 * Return hex string
 * @param data
 * @return
 */
std::string hexString(const std::string &data)
{
	return hexString((void *) data.c_str(), data.size());
}


std::string readHex(std::istream &s)
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

std::string hex2string(const std::string &hex)
{
	std::stringstream ss(hex);
    return readHex(ss);
}

// base64url
#define RB64U_RXBUFSZ (4)

typedef struct b64ue b64ue_t;
typedef struct b64ud b64ud_t;

struct b64ue
{
  uint8_t   f;
  size_t    q, i, j, n;
  uint32_t  b[RB64U_RXBUFSZ], t, k;
  char r;
};

struct b64ud
{
  uint8_t   f, s, k, q;
  uint32_t  a, b, c, t;
  char r;
};


void base64url_encode_reset(b64ue_t *state);
int base64url_encode_ingest(b64ue_t *state, char c);
int base64url_encode_getc(b64ue_t *state);
int base64url_encode_finish(b64ue_t *state);

void base64url_decode_reset(b64ud_t *state);
int base64url_decode_ingest(b64ud_t *state, unsigned char c);
int base64url_decode_getc(b64ud_t *state);
int base64url_decode_finish(b64ud_t *state);

static const char base64url_etab[64] =
{
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '-', '_'};


static const char base64url_dtab[256] = {
  '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\x01', '\x00', '\x00', '\x00', '\x5c', '\x08', '\x00', '\x00', '\xc0', '\x99', '\x76', '\xb7', '\xe0', '\x96', '\x76', '\xb7',
  '\x45', '\x82', '\x04', '\x08', '\x8c', '\x1a', '\x61', '\xb7', '\xc8', '\x81', '\x04', '\x08', '\x01', '\x3e', '\x00', '\x00',
  '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3a', '\x3b', '\x3c', '\x3d', '\x78', '\xb7', '\xe0', '\xe3', '\xff', '\xbf',
  '\xcf', '\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0a', '\x0b', '\x0c', '\x0d', '\x0e',
  '\x0f', '\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\xb7', '\x01', '\x00', '\x00', '\x3f',
  '\x00', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f', '\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28',
  '\x29', '\x2a', '\x2b', '\x2c', '\x2d', '\x2e', '\x2f', '\x30', '\x31', '\x32', '\x33', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\xd0', '\xe3', '\xff', '\xbf', '\xc4', '\xe3', '\xff', '\xbf', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\x00', '\x00', '\x00', '\x00', '\x10', '\xe4', '\xff', '\xbf', '\x68', '\x66', '\x78', '\xb7', '\x45', '\x82', '\x04', '\x08',
  '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
  '\x66', '\xf5', '\xff', '\xbf', '\x1e', '\x31', '\x67', '\xb7', '\x79', '\x0b', '\x72', '\xb7', '\xbc', '\x96', '\x04', '\x08',
  '\xb8', '\xe3', '\xff', '\xbf', '\xec', '\x82', '\x04', '\x08', '\xf4', '\xbf', '\x75', '\xb7', '\xbc', '\x96', '\x04', '\x08'};



int base64url_encode(char *dest, const size_t maxlen, const char *src, const size_t len, size_t *dlen)
{
  int r;
  size_t i,
         lost = 0,
         dsz = 0;
  b64ue_t s;
  if (NULL != dlen) *dlen = 0;
  base64url_encode_reset(&s);
  for (i = 0; i < len; i++)
  {
    r = base64url_encode_ingest(&s, src[i]);
    if (r < 0) {
      if (NULL != dlen) *dlen = dsz;
      return -1;
    }
    if (r > 0) {
      if (maxlen > dsz)
        dest[dsz++] = base64url_encode_getc(&s);
      else
        lost++;
    }
  }
  for (;;) {
    r = base64url_encode_finish(&s);
    if (r < 0) return -1;
    if (r > 0) {
      if (maxlen > dsz)
        dest[dsz++] = base64url_encode_getc(&s);
      else
        lost++;
    }
    else
      break;
  }
  if (NULL != dlen) *dlen = dsz;
  if (lost > 0) {
    /*no support for %z in C90 and no need for output anyway */
    /*fprintf(stderr, "base64url_encode: dropped %zu bytes to avoid output buffer overrun.", lost);*/
    return -1;
  }
  return 0;
}


int base64url_decode(char *dest, const size_t maxlen, const char *src, const size_t len, size_t *dlen)
{
  int r;
  size_t i,
         lost = 0,
         dsz = 0;
  b64ud_t s;
  if (NULL != dlen) *dlen = 0;
  base64url_decode_reset(&s);
  for (i = 0; i < len; i++)
  {
    r = base64url_decode_ingest(&s, src[i]);
    if (r < 0) {
      if (NULL != dlen) *dlen = dsz;
      return -1;
    }
    if (r > 0) {
      if (maxlen > dsz)
        dest[dsz++] = base64url_decode_getc(&s);
      else
        lost++;
    }
  }
  for (;;) {
    r = base64url_decode_finish(&s);
    if (r < 0) {
      if (NULL != dlen) *dlen = dsz;
      return -1;
    }
    if (r > 0) {
      if (maxlen > dsz)
        dest[dsz++] = base64url_decode_getc(&s);
      else
        lost++;
    }
    else
      break;
  }
  if (NULL != dlen) *dlen = dsz;
  if (lost > 0) {
    /*no support for %z in C90 and no need for output anyway */
    /*fprintf(stderr, "base64url_decode: dropped %zu bytes to avoid output buffer overrun.", lost);*/
    return -1;
  }
  return 0;
}


/* re-entrant methods *********************************************************/

int base64url_encode (char *dest, const size_t maxlen, const char *src, const size_t len, size_t *dlen);
int base64url_decode (char *dest, const size_t maxlen, const char *src, const size_t len, size_t *dlen);

void base64url_encode_reset(b64ue_t *state)
{
  state->f = 1;    /* unnamed state flag */
  state->k = 4;    /* triple shift */
  state->t = 0;    /* triple */
  state->i = 0;    /* write index */
  state->j = 0;    /* read index */
  state->q = 0;    /* total bytes read */
  state->n = 0;    /* bytes in buffer, b */
  state->b[0] = 0; /* read buffer */
  state->b[1] = 0;
  state->b[2] = 0;
  state->b[3] = 0;
}


int base64url_encode_getc(b64ue_t *state)
{
  int r = state->r;
  state->r = 0;
  return r;
}


int base64url_encode_ingest(b64ue_t *state, char c)
{
  size_t    i, j, n;
  uint32_t  t, k;

  i = state->i;
  state->b[i] = c;
  state->i = (i > RB64U_RXBUFSZ-2) ? 0 : i + 1;
  state->n++;
  state->q++;

  if (state->f > 0) {
    if (state->n < 3)
      return 0;
    state->f = 0;
  }

  t = state->t;
  k = state->k;
  if (k > 3) {
    k = 3;
    n = state->n;
    j = state->j;
    t =  (state->b[j] << 0x10);
    state->b[j] = 0;
    j = (j > RB64U_RXBUFSZ-2) ? 0 : j + 1;
    t += (state->b[j] << 0x08);
    state->b[j] = 0;
    j = (j > RB64U_RXBUFSZ-2) ? 0 : j + 1;
    t += (state->b[j]);
    state->b[j] = 0;
    j = (j > RB64U_RXBUFSZ-2) ? 0 : j + 1;
    state->j = j;
    state->t = t;
    state->n = n - 3;
  }

  state->r = base64url_etab[(t >> k * 6) & 0x3f];
  state->k = k - 1;
  return 1;
}


int base64url_encode_finish(b64ue_t *state)
{
  size_t    j, n;
  uint32_t  t, k;
  uint8_t   m;

  n = state->n;

  if (state->f > 0) {
    if (n < 1)
      return 0;
    state->f = 0;
  }

  t = state->t;
  k = state->k;
  if (k > 3) {
    if (n < 1)
      return 0;
    k = 3;
    j = state->j;
    t =  (state->b[j] << 0x10);
    state->b[j] = 0;
    j = (j > RB64U_RXBUFSZ-2) ? 0 : j + 1;
    t += (state->b[j] << 0x08);
    state->b[j] = 0;
    j = (j > RB64U_RXBUFSZ-2) ? 0 : j + 1;
    t += (state->b[j]);
    state->b[j] = 0;
    j = (j > RB64U_RXBUFSZ-2) ? 0 : j + 1;
    state->j = j;
    state->t = t;
    state->n = 0;
  }

  if (state->n == 0) {
    m = state->q % 3;
    if (((m == 1) && (k <= 1)) ||
        ((m == 2) && (k < 1)))
      return 0;
  }

  state->r = base64url_etab[(t >> k * 6) & 0x3f];
  state->k = k - 1;
  return 1;
}


void base64url_decode_reset(b64ud_t *state)
{
  state->f = 0; /* unnamed state flag */
  state->s = 0; /* buffer state */
  state->k = 3; /* output state */
  state->q = 0; /* runoff quota */
  state->r = 0; /* next return char */
  state->a = 0; /* buffer */
  state->b = 0; /* buffer */
  state->c = 0; /* buffer */
}


int base64url_decode_getc(b64ud_t *state)
{
  int r = state->r;
  state->r = 0;
  return r;
}


int base64url_decode_ingest(b64ud_t *state, unsigned char c)
{
  switch (state->s)
  {
  case 0:
    state->a = base64url_dtab[c];
    state->r = (state->t >> 1 * 8) & 0xff;
    state->k = 0;
    state->q = 3;
    state->s = 1;
    return state->f;

  case 1:
    state->b = base64url_dtab[c];
    state->r = (state->t >> 0 * 8) & 0xff;
    state->k = 2;
    state->q = 1;
    state->s = 2;
    return state->f;

  case 2:
    state->c = base64url_dtab[c];
    state->q = 2;
    state->s = 3;
    return 0;

  case 3:
    state->f = 1;
    state->t = (state->a << 3 * 6)
             + (state->b << 2 * 6)
             + (state->c << 1 * 6)
             + (base64url_dtab[c] << 0 * 6);
    state->a = 0;
    state->b = 0;
    state->c = 0;
    state->r = (state->t >> 2 * 8) & 0xff;
    state->k = 1;
    state->q = 3;
    state->s = 0;
    return 1;
  }
  return -1;
}


int base64url_decode_finish(b64ud_t *state)
{
  uint8_t k;
  k = state->k;

  if (k > 2)
    return 0;

  if (k > 1)
    state->t = (state->a << 3 * 6)
             + (state->b << 2 * 6)
             + (state->c << 1 * 6);

  if (state->q-- == 0)
    return 0;

  state->r = (state->t >> k * 8) & 0xff;
  state->k = k - 1;
  return 1;
}

std::string Base64UrlEncode(const void *value, size_t size)
{
	size_t dlen;
	size_t dest_sz = size * 3;
	char *dest = (char *) malloc(dest_sz);
	base64url_encode(dest, dest_sz, (char *) value, size, &dlen);
	return std::string(dest, dlen);
}


std::string base64urlencode(const std::string &value)
{
	return Base64UrlEncode(value.c_str(), value.size());
}

std::string base64urldecode(const std::string & value)
{
	size_t dlen;
	size_t sz = value.size();
	size_t dest_sz = sz;
	char *dest = (char *) malloc(dest_sz);
	base64url_decode(dest, dest_sz, value.c_str(), sz, &dlen);
	return std::string(dest, dlen);
}
