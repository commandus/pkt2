#ifndef HUFFMANMODIFIEDENCODER_H
#define HUFFMANMODIFIEDENCODER_H

#include <string>
#include <vector>
#include "huffcode.h"

class HuffmanModifiedEncoder
{
private:
 	int mode;																			///< 0- no compression, 1- modified Huffman. Default 1
	HuffCodeMap mCodeMap;
	HuffCode mEscapeCode;
public:
	HuffmanModifiedEncoder();
	HuffmanModifiedEncoder *setMode(int mode);											///< 0- no compression, 1- modified Huffman
	HuffmanModifiedEncoder *setEscapeCode(const std::string &escape_code);				///< for mode 1
	HuffmanModifiedEncoder *setCodeMapFromFrequencies(size_t *frequencies, size_t len);	///< array
	HuffmanModifiedEncoder *setCodeMapFromFrequenciesStream(std::istream *strm);		///< stream
	HuffmanModifiedEncoder *setCodeMapFromFrequenciesFile(const std::string &value);	///< file name
	HuffmanModifiedEncoder *setCodeMapFromStream(std::istream *strm);					///< stream
	HuffmanModifiedEncoder *setCodeMapFromFile(const std::string &value);				///< file name
	
	~HuffmanModifiedEncoder();

	/**
	 * @brief pack buffer to stream
	 * @param retval Destination buffer. Can be NULL
	 * @param src input buffer
	 * @param size Data size
	 * @return size
	 */
	size_t pack(std::ostream *retval, const void *src, size_t size, size_t offset);

	/**
	 * @brief Encode buffer to pre-allocated buffer or just return required size (if dest is NULL)
	 * @param dest Destination buffer. Can be NULL
	 * @param dest_size buffer size Can be 0
	 * @param value Data to pack
	 * @param size Data size
	 * @return size
	 */
	size_t encode_buffer2buffer(void *dest, size_t dest_size, const void *value, size_t size, size_t offset);
	
	/**
	 * @brief Encode buffer to string
	 * @param value Data to pack
	 * @param size Data size
	 * @return decoded value as string
	 */
	std::string encode_buffer2string(const void *value, size_t size, size_t offset);

	/**
	 * @brief Encode string to string
	 * @param value Data to pack
	 * @return decoded value as string
	 */
	std::string encode_string2string(const std::string &value, size_t offset);
};

#endif // HUFFMANMODIFIEDDECODER_H

