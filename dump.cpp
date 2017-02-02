/*
 * Dump.cpp
 *
 *  Created on: 28.10.2015
 *      Author: andrei
 */

#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include "dump.h"
#include "iridiumpacket.h"

// http://stackoverflow.com/questions/673240/how-do-i-print-an-unsigned-char-as-hex-in-c-using-ostream
struct HexCharStruct
{
	unsigned char c;
	HexCharStruct(unsigned char _c) : c(_c) { }
};

struct BinCharStruct
{
	unsigned char c;
	BinCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs)
{
	return (o << std::setfill('0') << std::setw(2) << std::hex << (int) hs.c);
}

inline std::ostream& operator<<(std::ostream& o, const BinCharStruct& hs)
{
	for (int i = 7; i >= 0; i--)
		o << (((hs.c >> i) & 1) != 0 ? "1" : "0");
	return o;
}

inline HexCharStruct hex(unsigned char c)
{
	return HexCharStruct(c);
}

inline BinCharStruct bin(unsigned char c)
{
	return BinCharStruct(c);
}

/**
  *	Debug helper function print out hex formatted buffer
  */
void bufferPrintHex(std::ostream &sout, const PacketRawBuffer* value)
{
	for (PacketRawBuffer::const_iterator it = value->begin(); it != value->end(); ++it)
		sout << hex(*it);
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

void bufferPrintBin(std::ostream &sout, void* value, size_t size)
{
	if (value == NULL)
		return;
	unsigned char *p = (unsigned char*) value;
	for (size_t i = 0; i < size; i++)
	{
		sout << bin(*p);
		p++;
	}
}

void printBuffer(void* value, size_t size)
{
	bufferPrintHex(std::cout, value, size);
}

void dumpInfoElementHeader(std::ostream &s, DumpStyle style, InfoElementHeader *v)
{
	switch (style)
	{
	case DS_VERBOSE:
		s << "version: " << v->id << "size: " << v->size << std::endl;
		break;
	default:
		s << v->id << " " << v->size << std::endl;
	}
}

void dumpInfoElement(std::ostream &s, DumpStyle style, InfoElement *v)
{
	dumpInfoElementHeader(s, style, &v->elementheader);
	switch (style)
	{
	case DS_VERBOSE:
		// s << "version: " << v->version << "size: " << v->size << std::endl;
		break;
	default:
		// s << v->version << " " << v->size << std::endl;
		break;
	}
}

Dump::Dump()
{
	// TODO Auto-generated constructor stub

}

Dump::~Dump()
{
	// TODO Auto-generated destructor stub
}

PacketRawBuffer Dump::readHex(std::istream &s)
{
	PacketRawBuffer r;
	s >> std::noskipws;
	char c[3] = {0, 0, 0};
	while (s >> c[0]) {
		if (!(s >> c[1]))
			break;
		unsigned char x = (unsigned char) strtol(c, NULL, 16);
		r.push_back(x);
	}
	return r;
}

PacketRawBuffer Dump::readHexStr(const std::string &s)
{
	std::stringstream ss(s);
	return readHex(ss);

}

std::string Dump::hexString(std::istream &s)
{
	s >> std::noskipws;
	std::stringstream r;
	unsigned char c;
	while (s >> c) {
		bufferPrintHex(r, &c, 1);
	}
	return r.str();
}

std::string Dump::hexString(void *buffer, size_t size)
{
	std::stringstream r;
	bufferPrintHex(r, buffer, size);
	return r.str();
}

std::string itoa(int v)
{
	std::stringstream ss;
	ss << v;
	return ss.str();
}
