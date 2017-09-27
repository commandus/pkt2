#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "huffmanmodifieddecoder.h"

HuffmanModifiedDecoder::HuffmanModifiedDecoder
(
	int amode,									///< 0- no compression, 1- modified Huffman
	size_t aoffset,								///< offset where data compression begins
	const std::string &adictionary_file_name,	///< file name
	const std::string &acodemap_file_name,		///< file name
	size_t abuffer_size
)
:	mode(amode), offset(aoffset), 
		dictionary_file_name(adictionary_file_name), 
		codemap_file_name(acodemap_file_name), 
		buffer_size(abuffer_size)
{
	if (buffer_size > 0)
		buffer = std::malloc(buffer_size);
	else
		buffer = NULL;

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
	
	if (acodemap_file_name.empty())
	{
		Node* root = buildTree(frequencies, UniqueSymbols);
		generateCodes(codeMap, root, HuffCode());
		delete root;
	}
	else
	{
		std::istream *strm = new std::ifstream(acodemap_file_name.c_str(), std::ifstream::in);
		loadCodeMap(codeMap, strm);
		delete strm;
	}
	// printCodeMap(std::cout, codes);
}

HuffmanModifiedDecoder::~HuffmanModifiedDecoder()
{
	if (buffer)
		free(buffer);
	buffer = NULL;
}

size_t HuffmanModifiedDecoder::decompress(void *dest, void *src, size_t size)
{
	if (mode != 1) 
	{
		std::memmove(dest, src, size);
		return size;
	}
	// huffman
}

size_t HuffmanModifiedDecoder::decode(void *data, size_t len, size_t data_size)
{
	switch (mode) 
	{
		case 0: 
			return len;
		default:
		{
			if (!data)
				return 0;
			size_t sz = decompress(buffer, (char *) data + offset, len - offset);
			if (sz > 0)
			{
				if (sz > data_size)
					sz = data_size;
				if (sz > 0)
				{
					/* Already
					if (offset > 0)
						std::memmove(data, buffer, offset);
					*/
					std::memmove((char *) data + offset, buffer, sz);
				}
			}
			return sz + offset;
		}
	}
}
