#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "huffmanmodifieddecoder.h"
#include "huffcode.h"

HuffmanModifiedDecoder::HuffmanModifiedDecoder
(
	int amode,									///< 0- no compression, 1- modified Huffman
	size_t aoffset,								///< offset where data compression begins
	const std::string &adictionary_file_name,	///< file name
	const std::string &acodemap_file_name,		///< file name
	const std::vector<size_t> &a_valid_packet_sizes
)
:	mode(amode), offset(aoffset), 
		dictionary_file_name(adictionary_file_name), 
		codemap_file_name(acodemap_file_name), 
		valid_packet_sizes(a_valid_packet_sizes)
{
	// Build frequency table
	if (adictionary_file_name.empty())
	{
		std::stringstream alphabet;
		UniqueSymbols = 256;
		for (int i = 0; i < UniqueSymbols; i++) 
		{
			alphabet << (unsigned char) i;
		}
		buildFrequencies(frequencies, UniqueSymbols, alphabet.str().c_str(), UniqueSymbols);
	}
	else
	{
		std::istream *strm = new std::ifstream(adictionary_file_name.c_str(), std::ifstream::in);
		UniqueSymbols = loadFrequencies(frequencies, 256, strm);
		delete strm;
	}
	
	HuffCodeMap codeMap;
	if (acodemap_file_name.empty())
	{
		mRoot = buildTree(frequencies, UniqueSymbols);
		generateCodes(codeMap, mRoot, HuffCode());
	}
	else
	{
		std::istream *strm = new std::ifstream(acodemap_file_name.c_str(), std::ifstream::in);
		loadCodeMap(codeMap, strm);
		delete strm;
		HuffCodeMap codes;
		mRoot = buildTreeFromCodes(codes);
	}
	// printCodeMap(std::cout, codes);
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
 	HuffCode escape_code;
	size_t sz = decompress(retval, mRoot, escape_code, offset, src, size); 
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
