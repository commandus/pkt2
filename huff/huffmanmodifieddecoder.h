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
	
	size_t frequencies[256];
	int UniqueSymbols;
	std::vector<size_t> valid_packet_sizes;
	Node *mRoot;
public:
	HuffmanModifiedDecoder(
		int mode,									///< 0- no compression, 1- modified Huffman
		size_t offset,								///< offset where data compression begins
		const std::string &dictionary_file_name,	///< file name
		const std::string &code_map_file_name,		///< file name
		const std::vector<size_t> &valid_packet_sizes
	);
	~HuffmanModifiedDecoder();

	/**
	 * @brief Unpack buffer to stream
	 * @param retval Destination buffer. Can be NULL
	 * @param src input buffer
	 * @param len You nedd to know unpacket(original) size)
	 * @return size
	 */
	size_t unpack(std::ostream *retval, void *src, size_t len, size_t offset);
	/**
	 * @brief Decode buffer to pre-allocated buffer or just return required size (if dest is NULL)
	 * @param dest Destination buffer. Can be NULL
	 * @param dest_size buffer size Can be 0
	 * @param value Data to unpack
	 * @param len You nedd to know unpacket(original) size)
	 * @return size
	 */
	size_t decode_buffer2buffer(void *dest, size_t dest_size, void *value, size_t len, size_t offset);
	/**
	 * @brief Decode buffer to string
	 * @param value Data to unpack
	 * @param len You nedd to know unpacket(original) size)
	 * @return decoded value as string
	 */
	std::string decode_buffer2string(void *value, size_t len, size_t offset);
	/**
	 * @brief Decode string to string
	 * @param value Data to unpack
	 * @return decoded value as string
	 */
	std::string decode_string2string(const std::string &value, size_t len, size_t offset);
};

#endif // HUFFMANMODIFIEDDECODER_H
