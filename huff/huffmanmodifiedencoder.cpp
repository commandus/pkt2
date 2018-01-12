#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "huffmanmodifiedencoder.h"
#include "huffcode.h"

HuffmanModifiedEncoder::HuffmanModifiedEncoder()
	:	mode(1)
{
	mCodeMap = defaultHuffmanCodeMap();
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setCodeMapByMode
(
	int value		///< 0- no compression, 1- modified Huffman
)
{
	mode = value;
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setEscapeCode
(
	const std::string &escape_code,				///< for mode 1
	int bits
)
{
	mEscapeCodeSizes.push_back(HuffCodeNSize(getHuffCode(escape_code), bits)); 
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setEOFCode
(
	const std::string &escape_code				///< for mode 1
)
{
	if (escape_code.empty())
		mEOFCode = NULL;
	else
	{
		mEOFCodeData = getHuffCode(escape_code);
		mEOFCode = &mEOFCodeData; 
	}
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setCodeMapFromFrequencies(size_t *frequencies, size_t len)
{
	Node* root = buildTree(frequencies, len);
	mCodeMap.clear();
	generateCodes(mCodeMap, root, HuffCode());
	delete root;
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setCodeMapFromFrequenciesStream(std::istream *strm)
{
	size_t frequencies[256];
	loadFrequencies(frequencies, 256, strm);
	setCodeMapFromFrequencies(frequencies, 256);
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setCodeMapFromStream(std::istream *strm)
{
	loadCodeMap(mCodeMap, strm);
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setCodeMapFromFrequenciesFile(const std::string &value)
{
	std::istream *strm = new std::ifstream(value.c_str(), std::ifstream::in);
	setCodeMapFromFrequenciesStream(strm);
	delete strm;
	return this;
}

HuffmanModifiedEncoder *HuffmanModifiedEncoder::setCodeMapFromFile(const std::string &value)
{
	std::istream *strm = new std::ifstream(value.c_str(), std::ifstream::in);
	setCodeMapFromStream(strm);
	delete strm;
	return this;
}

HuffmanModifiedEncoder::~HuffmanModifiedEncoder()
{
}

/**
	* @brief pack buffer to stream
	* @param retval Destination buffer. Can be NULL
	* @param src input buffer
	* @param size Data size
	* @return size
	*/
size_t HuffmanModifiedEncoder::pack
(
	std::ostream *retval,
	const void *src,
	size_t size,
	size_t offset
)
{
	if (mode != 1) 
	{
		retval->write((const char *) src, size);
		return size;
	}
	return compress(retval, mCodeMap, mEscapeCodeSizes, mEOFCode, offset, src, size); 
}

/**
	* @brief Encode string to string
	* @param value Data to pack
	* @return decoded value as string
	*/
std::string HuffmanModifiedEncoder::encode_string2string
(
	const std::string &value, 
	size_t offset
)
{
	// get size
	size_t sz = encode_buffer2buffer(NULL, 0, value.c_str(), value.size(), offset);
	if (sz == 0)
		return "";
	std::string r('\0', sz);
	encode_buffer2buffer((void *) r.c_str(), sz, value.c_str(), value.size(), offset);
	return r;
}

/**
	* @brief Encode buffer to pre-allocated buffer or just return required size (if dest is NULL)
	* @param dest Destination buffer. Can be NULL
	* @param dest_size buffer size Can be 0
	* @param data Data to pack
	* @param size Data size
	* @return size
	*/
size_t HuffmanModifiedEncoder::encode_buffer2buffer
(
	void *dest, 
	size_t dest_size, 
	const void *data, 
	size_t size,
	size_t offset
)
{
	if (!data)
		return 0;
	size_t sz;
	switch (mode) 
	{
		case 0:
			// copy buffer
			sz = size;
			if (sz > dest_size)
				sz = dest_size;
			if (dest)
				std::memmove((char *) dest + offset, data, sz);
			return size;
		default:
		{
			std::stringstream ss;
			sz = pack(&ss, (char *) data + offset, size - offset, offset);
			if (sz > 0)
			{
				if (dest)
				{
					// truncate buffer to maximum size if exceed
					if (sz > dest_size)
						sz = dest_size;
					std::string s = ss.str();
						std::memmove((char *) dest + offset, s.c_str(), sz);
				}
			}
			return sz + offset;
		}
	}
}

const HuffCodeMap& HuffmanModifiedEncoder::getCodeMap()
{
	return mCodeMap;
}
