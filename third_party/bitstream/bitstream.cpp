#include "bitstream.h"
#include "varint.h"

//------------------------------------ ibitstream ------------------------------------


/**
* Return false- EOF
**/
bool ibitstream::read_buffer()
{
	if (strm->eof())
		return false;
	strm->read((char *) &buffer, 1);
	bit_count = 0;
}

ibitstream::ibitstream(std::istream *astrm)
	:	strm(astrm)
{
	bit_count = 8;	// indicate no data read yet
}

ibitstream::~ibitstream()
{
}

bool ibitstream::has_bit()
{
	return bit_count < 8;
}

/**
  * Return 0, 1, -1(EOF)
  */
int ibitstream::read()
{
	if (!has_bit())
		if (!read_buffer())
			return -1;
	int r = (buffer & (1 << bit_count)) ? 1 : 0;
	bit_count++;
	return r;
}

/**
  * Return 8 bits as char
  */
char ibitstream::read8()
{
	char r = 0;
	for (int i = 0; i < 8; i++)
	{
		int b = read();
		if (b == 1)
			r = r | (1 << i);
	}
	return r;
}

//------------------------------------ obitstream ------------------------------------

void obitstream::new_buffer()
{
	buffer = 0;
	bit_count = 0;
}

void obitstream::flush()
{
	if (bit_count)
	{
		strm->write((const char *) &buffer, 1);
		new_buffer();
	}
}

obitstream::obitstream(std::ostream *astrm)
	:	strm(astrm)
{
	new_buffer();
}

obitstream::~obitstream()
{
	flush();
}

bool obitstream::is_filled()
{
	return bit_count >= 8;
}

void obitstream::write_bit
(
	int bit
)
{
	// std::cerr << (bit ? "1" : "0");
	if (bit) 
		buffer |= 1 << bit_count;
	else
		buffer &= ~(1 << bit_count);
	bit_count++;
}

void obitstream::write
(
	int bit
)
{
	if (is_filled())
	{
		flush();
	}
	write_bit(bit);
}

void obitstream::write
(
	const std::vector<bool> &value
)
{
	for (std::vector<bool>::const_iterator it(value.begin()); it != value.end(); it++)
	{
		write(*it);
	}
}

void obitstream::write
(
	int value, 
	int bits
)
{
	for (int i = 0; i < bits; i++)
	{
		write(value & (1 << i));
	}
}

/**
 * Read variable length integer.
 * Return 0 if incomplete(no room)
 */
uint64_t ibitstream::read_varint
(
	std::istream *strm
)
{
	uint8_t buf[8];
	size_t size = 0;
	for (int i = 0; i < 8; i++)
	{
		strm->read((char *) &buf[i], 1);
		size++;
		if (is_varint_all_bytes<uint64_t>((uint8_t *) &buf, size))
		{
			break;
		}
	}
	if (size >= 8)
		return 0;
	else
		return decodeVarint<uint64_t>((uint8_t*) &buf, size);
}

/**
 * Get variable length integer fron the buffer.
 * @param data buffer pointer
 * @param bytes_read can be NULL
 * @return 0 if incomplete(no room)
 */
uint64_t ibitstream::get_varint
(
	uint8_t *data,
	size_t *bytes_read
)
{
	size_t size = 0;
	for (int i = 0; i < 8; i++)
	{
		size++;
		if (is_varint_all_bytes<uint64_t>((uint8_t *) data, size))
		{
			break;
		}
	}
	if (size >= 8)
		size = 0;
	if (bytes_read)
		*bytes_read = size;
	if (size > 0)
		return decodeVarint<uint64_t>(data, size);
	else
		return 0;
}

/**
 * Write variable length integer
 * Return occupied bytes
 */
size_t obitstream::write_varint
(
	std::ostream *strm,
	uint64_t value
)
{
	uint8_t buf[4];
	size_t outsize = encodeVarint<uint64_t>(value, (uint8_t*) &buf);
	if (outsize)
	{
		strm->write((char *) &buf, outsize); 
	}
	return outsize;
}
