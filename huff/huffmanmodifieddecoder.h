#ifndef HUFFMANMODIFIEDDECODER_H
#define HUFFMANMODIFIEDDECODER_H

#include <string>
#include <vector>
#include "huffcode.h"

class HuffmanModifiedDecoder
{
private:
	int mode;													///< 0- no compression, 1- modified Huffman. Default 1
	size_t offset;												///< offset where data compression begins
	std::vector<size_t> valid_packet_sizes;
	Node *mRoot;
	HuffCode mEscapeCode;
public:
	HuffmanModifiedDecoder();
	HuffmanModifiedDecoder *setMode(int mode);										///< 0- no compression, 1- modified Huffman
	HuffmanModifiedDecoder *setEscapeCode(const std::string &escape_code);			///< for mode 1
	HuffmanModifiedDecoder *setOffset(size_t offset);								///< offset where data compression begins
	HuffmanModifiedDecoder *setTreeFromFrequencies(size_t *frequencies, size_t len);		///< array
	HuffmanModifiedDecoder *setTreeFromFrequenciesStream(std::istream *strm);		///< stream
	HuffmanModifiedDecoder *setTreeFromCodesStream(std::istream *strm);			///< stream
	HuffmanModifiedDecoder *setTreeFromFrequenciesFile(const std::string &value);	///< file name
	HuffmanModifiedDecoder *setTreeFromCodesFile(const std::string &value);		///< file name
	HuffmanModifiedDecoder *setValidPacketSizes
	(
		const std::vector<size_t> &valid_packet_sizes			///< can be empty means do not validate
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
