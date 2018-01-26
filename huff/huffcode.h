/*
 * @file huffcode.h
 */
#include <cstddef>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <map>

#include "node.h"
#include "internalnode.h"
#include "leafnode.h"

typedef std::vector<bool> HuffCode;
typedef std::map<unsigned char, HuffCode> HuffCodeMap;
typedef std::pair<HuffCode, int> HuffCodeNSize;
typedef std::vector<HuffCodeNSize> HuffCodeNSizes;

void buildFrequencies
(
	size_t *frequencies, 
	size_t symbols_size,
	const void *data,
	size_t data_size
);

size_t loadFrequencies
(
	size_t *frequencies,
	size_t symbols_size,
	std::istream *strm
);

void printFrequencies
(
	std::ostream &strm,
	size_t *frequencies,
	size_t symbols_size,
	bool include_zeroes
);

Node* buildTree
(
	const size_t *frequencies, 
	size_t symbols_size
);

Node* buildTreeFromCodes
(
	const HuffCodeMap& codes
);

/**
 * @brief Return maximum depth of the Huffman tree
 * @param node Huffman tree root node
 * @return maximum depth of the Huffman tree
 */
int get_tree_max_depth
(
	const Node* node
);

/**
 * @brief Dumps a representation of the tree to cout
 * TODO there are smth wrong with https://stackoverflow.com/questions/36802354/print-binary-tree-in-a-pretty-way-using-c
 */
void dump_tree
(
	Node *node
);
 
/**
 * @brief print out Huffman codes tree for debug purposes
 * @param strm output stream
 * @param node Huffman codes tree
 * @param prefix Huffman code prefix
 */
void print_tree
(
	int level,
	std::ostream &strm,
	const Node* node,
	const HuffCode &prefix
);

void generateCodes
(
	HuffCodeMap &outCodes,
	const Node *node, 
	const HuffCode &prefix 
);

void printCodeMap
(
	std::ostream &strm,
	const HuffCodeMap& codes
);

size_t loadCodeMap
(
	HuffCodeMap& codes,
	std::istream *strm
);

/**
 * @param retval stream to be compressed
 * @param codes Huffman codes
 * @param escape_code_sizes Huffman code and size
 * @param compression_offset offset in data buffer
 * @param data buffer
 * @param size buffer size 
 * @return bits
 */
size_t compress
(
	std::ostream *retval,
	const HuffCodeMap& codes,
	const HuffCodeNSizes &escape_code_sizes,
	const HuffCode *eof_code,  
	int compression_offset,
	const void *data, 
	size_t size
);

/**
 * @param mode 0- default, 1- faster
 * @param retval compressed stream
 * @param root Huffman codes tree root node
 * @param escape_code_sizes Huffman code and size vector
 * @param eof_code Can be NULL
 * @param force_size >0 do not use length descriptor or EOF code (force size)
 * @param compression_offset offset in data buffer
 * @param data buffer
 * @param size buffer size 
 * @return bytes
 */
size_t decompress
(
	int mode,
	std::ostream *retval,
	const Node *root,
	const HuffCodeNSizes &escape_code_sizes,
	const HuffCode *eof_code,  
	int force_size,
	int compression_offset,
	const void *data, 
	size_t size
);

/**
 * @param retval compressed stream
 * @param codes Huffman codes
 * @param escape_code_sizes Huffman code
 * @param force_size 0
 * @param compression_offset offset in data buffer
 * @param data buffer
 * @param size buffer size 
 * @return bytes
*/
size_t decompress2
(
	std::ostream *retval,
	HuffCodeMap& codes,
	const HuffCodeNSizes &escape_code_sizes, 
 	const HuffCode *eof_code,  
	int force_size,
	int compression_offset,
	const void *data, 
	size_t size
);

size_t encode_string
(
	std::string &retval,
	HuffCodeMap& codes,
	const void *data, 
	size_t size
);

/**
 * Read Huffman code from the binary text representaion
 * @param t binary text representation
 */
HuffCode getHuffCode
(
	const std::string &t
);

Node* defaultHuffmanCodeTree();

HuffCodeMap defaultHuffmanCodeMap();

Node* loadHuffmanCodeTreeFromFrequencyStream
(
	std::istream *frequencies_stream	///< stream
);

Node* loadHuffmanCodeTreeFromFrequencyFile
(
	const std::string &frequencies_file	///< file name
);

Node* loadHuffmanCodeTreeFromCodeStream
(
	std::istream *codes_stream	///< stream
);

Node* loadHuffmanCodeTreeFromCodeFile
(
	const std::string &codes_file	///< file name
);

bool checkTags(const void *data, size_t size, const std::vector<std::pair<int, int> > &tags);
