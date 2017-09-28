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

typedef int (*log_func)(int, int, const std::string&);

typedef std::vector<bool> HuffCode;
typedef std::map<unsigned char, HuffCode> HuffCodeMap;

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
	std::istream *strm,
	const log_func cblog = NULL
);

void printFrequencies
(
	std::ostream &strm,
	size_t *frequencies,
	size_t symbols_size,
	bool include_zeroes,
	const log_func cblog = NULL
);

Node* buildTree
(
	const size_t *frequencies, 
	size_t symbols_size
);

void generateCodes
(
	HuffCodeMap& outCodes,
	const Node* node, 
	const HuffCode& prefix 
);

void printCodeMap
(
	std::ostream &strm,
	const HuffCodeMap& codes,
	const log_func cblog = NULL
);

size_t loadCodeMap
(
	HuffCodeMap& codes,
	std::istream *strm,
	const log_func cblog = NULL
);

size_t calc_coded_size_bits
(
	HuffCodeMap& codes,
	const void *data, 
	size_t size,
 	const log_func cblog = NULL
);

size_t encode_string
(
	std::string &retval,
	HuffCodeMap& codes,
	const void *data, 
	size_t size
);
