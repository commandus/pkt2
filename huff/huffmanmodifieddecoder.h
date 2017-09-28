#ifndef HUFFMANMODIFIEDDECODER_H
#define HUFFMANMODIFIEDDECODER_H

#include <string>
#include <vector>
#include "huffcode.h"

class HuffmanModifiedDecoder
{
private:
	int mode;										///< 0- no compression, 1- modified Huffman
	size_t offset;									///< offset where data compression begins
	std::string dictionary_file_name;				///< file name
	std::string codemap_file_name;					///< file name
	size_t buffer_size;								///< allocated memory
	void *buffer;
	
	size_t frequencies[256];
	int UniqueSymbols;
	HuffCodeMap codeMap;
	std::vector<size_t> valid_packet_sizes;
		
	size_t decompress(void *dest, void *src, size_t size);
public:
	HuffmanModifiedDecoder(
		int mode,									///< 0- no compression, 1- modified Huffman
		size_t offset,								///< offset where data compression begins
		const std::string &dictionary_file_name,	///< file name
		const std::string &code_map_file_name,		///< file name
		size_t buffer_size,
		const std::vector<size_t> &valid_packet_sizes
	);
	~HuffmanModifiedDecoder();
	size_t decode(void *buffer, size_t len, size_t buffer_size);
};

#endif // HUFFMANMODIFIEDDECODER_H
