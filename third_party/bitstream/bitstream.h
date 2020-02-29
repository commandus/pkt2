#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <inttypes.h>
#include <iostream>
#include <vector>

class ibitstream 
{
private:
	int bit_count;
	unsigned char buffer;
	std::istream *strm;
	/**
	 * Return false- EOF
	 **/
	bool read_buffer();
	bool has_bit();
public:
	ibitstream
	(
		std::istream *strm
	);
	static uint64_t read_varint
	(
		std::istream *strm
	); 
	/**
	* Get variable length integer fron the buffer.
	* @param data buffer pointer
	* @param bytes_read can be NULL
	* @return 0 if incomplete(no room)
	*/
	static uint64_t get_varint
	(
		uint8_t *data,
		size_t *bytes_read
	);
	
	/**
	 * Return 0, 1, -1(EOF)
	 */
	int read();
	
	/**
	 * Return 8 bits as char
	 */
	char read8();
	virtual ~ibitstream();
};

class obitstream 
{
private:
	int bit_count;
	unsigned char buffer;
	std::ostream *strm;
	void new_buffer();
	bool is_filled();
	void write_bit(int bit);
public:
	obitstream(std::ostream *strm);
	static size_t write_varint(std::ostream *strm, uint64_t value);
	void write(int bit);
	void write(const std::vector<bool> &value);
	void write(int value, int bits);
	void flush();
	virtual ~obitstream();
};

#endif


