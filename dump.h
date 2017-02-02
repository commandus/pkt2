/*
 * Dump.h
 *
 *  Created on: 28.10.2015
 *      Author: andrei
 */

#ifndef DUMP_H_
#define DUMP_H_

#include <vector>
#include <string>
#include <iostream>
#include <istream>
#include <ostream>

typedef std::vector<unsigned char> PacketRawBuffer;

typedef enum
{
	DS_DEC = 0,
	DS_VERBOSE = 1
} DumpStyle;

void bufferPrintHex(std::ostream &sout, const PacketRawBuffer* value);
void bufferPrintHex(std::ostream &sout, void* value, size_t size);
void bufferPrintBin(std::ostream &sout, void* value, size_t size);

void printBuffer(void* value, size_t size);

class Dump
{
public:
	Dump();
	virtual ~Dump();
	static PacketRawBuffer readHex(std::istream &s);
	static PacketRawBuffer readHexStr(const std::string &s);
	static std::string hexString(std::istream &s);
	static std::string hexString(void *buffer, size_t size);
};

std::string itoa(int v);

#endif /* DUMP_H_ */
