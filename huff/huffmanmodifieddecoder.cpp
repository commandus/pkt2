#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "huffmanmodifieddecoder.h"
#include "huffcode.h"

HuffmanModifiedDecoder::HuffmanModifiedDecoder()
:	mode(1), offset(0)
{
	mRoot = defaultHuffmanCodeTree();
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setMode
(
	int value		///< 0- no compression, 1- modified Huffman
)
{
	mode = value;
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setEscapeCode
(
	const std::string &escape_code				///< for mode 1
)
{
	mEscapeCode = getHuffCode(escape_code); 
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setOffset
(
	size_t value								///< offset where data compression begins
)
{
	offset = value;
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setTreeFromFrequencies(size_t *frequencies, size_t len)
{
	if (mRoot)
		delete mRoot;
	mRoot = buildTree(frequencies, len);
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setTreeFromFrequenciesStream(std::istream *strm)
{
	mRoot = loadHuffmanCodeTreeFromFrequencyStream(strm);
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setTreeFromCodesStream(std::istream *strm)
{
	mRoot = loadHuffmanCodeTreeFromCodeStream(strm);
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setTreeFromFrequenciesFile(const std::string &value)
{
	mRoot = loadHuffmanCodeTreeFromFrequencyFile(value);
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setTreeFromCodesFile(const std::string &value)
{
	mRoot = loadHuffmanCodeTreeFromCodeFile(value);
	return this;
}

HuffmanModifiedDecoder *HuffmanModifiedDecoder::setValidPacketSizes
(
	const std::vector<size_t> &value			///< can be NULL
)
{
	valid_packet_sizes = value;
	return this;
}

HuffmanModifiedDecoder::~HuffmanModifiedDecoder()
{
	if (mRoot)
		delete mRoot;
}

size_t HuffmanModifiedDecoder::unpack
(
	std::ostream *retval,
	void *src,
	size_t size,
	size_t offset
)
{
	if (mode != 1) 
	{
		retval->write((const char *) src, size);
		return size;
	}
	// huffman
	size_t sz = decompress(retval, mRoot, mEscapeCode, offset, src, size); 
	return sz;
}

/**
	* @brief Decode buffer to string
	* @param value Data to unpack
	* @param len You nedd to know unpacket(original) size)
	* @return decoded value as string
	*/
std::string HuffmanModifiedDecoder::decode_buffer2string
(
	void *value, 
	size_t len,
	size_t offset
)
{
	// get size
	size_t sz = decode_buffer2buffer(NULL, 0, value, len, offset);
	if (sz == 0)
		return "";
	std::string r('\0', sz);
	decode_buffer2buffer((void *) r.c_str(), sz, value, len, offset);
	return r;
}

/**
	* @brief Decode string to string
	* @param value Data to unpack
	* @return decoded value as string
	*/
std::string HuffmanModifiedDecoder::decode_string2string
(
	const std::string &value, 
	size_t len,
	size_t offset
)
{
	return decode_buffer2string((void *) value.c_str(), len, offset);
}

/**
	* @brief Decode buffer to pre-allocated buffer or just return required size (if dest is NULL)
	* @param dest Destination buffer. Can be NULL
	* @param dest_size buffer size Can be 0
	* @param data Data to unpack
	* @param len You nedd to know unpacket(original) size)
	* @return size
	*/
size_t HuffmanModifiedDecoder::decode_buffer2buffer
(
	void *dest, 
	size_t dest_size, 
	void *data, 
	size_t len,
	size_t offset
)
{
	switch (mode) 
	{
		case 0: 
			return len;
		default:
		{
			if (!data)
				return 0;
			std::stringstream ss;
			size_t sz = unpack(&ss, (char *) data + offset, len - offset, offset);
			if (sz > 0)
			{
				// truncate buffer to maximum size if exceed
				if (sz > dest_size)
					sz = dest_size;
				// check allowed sizes
				if (valid_packet_sizes.size())
				{
					if (std::find(valid_packet_sizes.begin(), valid_packet_sizes.end(), sz + offset) != valid_packet_sizes.end())
						// if packet size is not in list, return as is(not decompressed)
						return len;
				}
				std::string s = ss.str();
				std::memmove((char *) dest + offset, s.c_str(), sz);
			}
			
			return sz + offset;
		}
	}
}
