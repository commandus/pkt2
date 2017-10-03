#include <iostream>
#include <iomanip>
#include <algorithm>
#include <inttypes.h>
#include <limits>
#include <queue>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iterator>

#include "huffcode.h"
#include "internalnode.h"
#include "leafnode.h"

#include "utilstring.h"

struct NodeCmp
{
    bool operator()(
		const Node* lhs, 
		const Node* rhs
	) const 
	{
		return lhs->f > rhs->f; 
	}
};
 
void buildFrequencies
(
	size_t *frequencies, 
	size_t symbols_size,
	const void *data,
	size_t data_size
)
{
	memset(frequencies, 0, sizeof(size_t) * symbols_size);
	for (int i = 0; i < data_size; ++i)
	{
		++frequencies[((const unsigned char *) data)[i]];
	}
}

size_t loadFrequencies
(
	size_t *frequencies,
	size_t symbols_size,
	std::istream *strm,
	const log_func cblog
)
{
	size_t r = 0;
	memset(frequencies, 0, sizeof(size_t) * symbols_size);
	std::string line;
	while (std::getline(*strm, line))
	{
		std::vector<uint64_t> inputs;
		std::istringstream in(line);
		std::copy(std::istream_iterator<uint64_t>(in), std::istream_iterator<uint64_t>(), std::back_inserter(inputs));
		if (inputs.size() >= 2)
		{
			if ((inputs[0] < symbols_size) && (inputs[0] >= 0))
			{
				frequencies[inputs[0]] = inputs[1];
			}
			else
			{
				if (cblog != NULL)
				{
					std::stringstream ss;
					ss << "First column is symbol number " << inputs[0] << " is not between 0 and " << symbols_size<< " in line: " << line << std::endl;
					cblog(0, 1, ss.str());
				}
				return r;
			}
			r++;
		}
	}
	return r;
}


void printFrequencies
(
	std::ostream &strm,
	size_t *frequencies,
	size_t symbols_size,
	bool include_zeroes,
	const log_func cblog
)
{
	for (int i = 0; i < symbols_size; ++i)
	{
		if (include_zeroes || (frequencies[i] != 0))
		{
			if (cblog != NULL)
			{
				std::stringstream ss;
				ss << std::dec << std::setw(3) << std::left << std::setfill(' ') << i << std::right << "\t"
					<< frequencies[i] << "\t"
					<< "0x" << std::setw(2) << std::setfill('0') << std::hex << i
					<< std::dec << std::endl;
				cblog(0, 1, ss.str());
			}
			else
			{
				strm << std::dec << std::setw(3) << std::left << std::setfill(' ') << i << std::right << "\t"
					<< frequencies[i] << "\t"
					<< "0x" << std::setw(2) << std::setfill('0') << std::hex << i
					<< std::dec << std::endl;
			}
		}
	}
}

Node* buildTree
(
	const size_t *frequencies, 
	size_t symbols_size
)
{
	std::priority_queue<Node*, std::vector<Node*>, NodeCmp> trees;

	for (int i = 0; i < symbols_size; ++i)
	{
		if (frequencies[i] != 0)
			trees.push(new LeafNode(frequencies[i], (char)i));
	}
	if (trees.size() == 0)
		return NULL;
	while (trees.size() > 1)
	{
		Node* childR = trees.top();
		trees.pop();

		Node* childL = trees.top();
		trees.pop();

		Node* parent = new InternalNode(childR, childL);
		trees.push(parent);
	}
	return trees.top();
}
 
void generateCodes
(
	HuffCodeMap& outCodes,
	const Node* node, 
	const HuffCode& prefix 
)
{
	if (const LeafNode* lf = dynamic_cast<const LeafNode*>(node))
	{
		outCodes[lf->c] = prefix;
	}
	else if (const InternalNode* in = dynamic_cast<const InternalNode*>(node))
	{
		HuffCode leftPrefix = prefix;
		leftPrefix.push_back(false);
		generateCodes(outCodes, in->left, leftPrefix);

		HuffCode rightPrefix = prefix;
		rightPrefix.push_back(true);
		generateCodes(outCodes, in->right, rightPrefix);
	}
}

void printCodeMap
(
	std::ostream &strm,
	const HuffCodeMap& codes,
	const log_func cblog
)
{
	for (HuffCodeMap::const_iterator it = codes.begin(); it != codes.end(); ++it)
	{
			if (cblog != NULL)
			{
				std::stringstream ss;
				ss << std::dec << std::setw(3) << std::left << std::setfill(' ') << (int) it->first << "\t" << std::right;
				std::copy(it->second.begin(), it->second.end(), std::ostream_iterator<bool>(std::cout));
				strm << "\t0x" << std::setw(2) << std::setfill('0') << std::hex << (int) it->first;
				strm << std::right << std::dec << std::endl;
				cblog(0, 1, ss.str());
			}
			else
			{
				strm << std::dec << std::setw(3) << std::left << std::setfill(' ') << (int) it->first << "\t" << std::right;
				std::copy(it->second.begin(), it->second.end(), std::ostream_iterator<bool>(std::cout));
				strm << "\t0x" << std::setw(2) << std::setfill('0') << std::hex << (int) it->first;
				strm << std::right << std::dec << std::endl;
			}
	}
}

// https://stackoverflow.com/questions/8362094/replace-multiple-spaces-with-one-space-in-a-string
bool BothAreSpaces(char lhs, char rhs) 
{
	return (lhs == rhs) && (lhs == ' '); 
}

size_t loadCodeMap
(
	HuffCodeMap& codes,
	std::istream *strm,
	const log_func cblog
)
{
	size_t r = 0;
	std::string line;
	while (std::getline(*strm, line))
	{
		std::replace(line.begin(), line.end(), '\t', ' ');
		std::string::iterator new_end = std::unique(line.begin(), line.end(), BothAreSpaces);
		line.erase(new_end, line.end());

		std::vector<std::string> t = split(line, ' ');
		int c = 0;
		
		if (t.size() < 2)
			continue;
		uint64_t symbol = strtod(t[0].c_str(), NULL);
		if (symbol < 255)
		{
			codes[symbol] = getHuffCode(t[1]);
		}
		else
		{
			if (cblog != NULL)
			{
				std::stringstream ss;
				ss << "First column is symbol number " << symbol << " is not between 0 and " << 255 << " in line: " << line << std::endl;
				cblog(0, 1, ss.str());
			}
			else
				std::cerr << "First column is symbol number " << symbol << " is not between 0 and " << 255 << " in line: " << line << std::endl;
			return r;
		}
		r++;
	}
	return r;
}

size_t calc_coded_size_bits
(
	HuffCodeMap& codes,
	const void *data, 
	size_t size,
 	const log_func cblog
)
{
	size_t r = 0;
	for (int i = 0; i < size; ++i)
	{
		std::map<unsigned char, HuffCode>::const_iterator it = codes.find(((unsigned char*) data)[i]);
		if (it == codes.end())
		{
			if (cblog != NULL)
			{
				std::stringstream ss;
				ss << "Error: no code for " << (int)((unsigned char*) data)[i]  << " 0x" << std::hex << std::setw(2) << (int)((unsigned char*) data)[i] << std::dec << std::endl;
				cblog(0, 1, ss.str());
			}
			else
				std::cerr << "Error: no code for " << (int)((unsigned char*) data)[i]  << " 0x" << std::hex << std::setw(2) << (int)((unsigned char*) data)[i] << std::dec << std::endl;

			return std::numeric_limits<uint64_t>::max();
		}
		r += it->second.size();
	}
	return r;
}


size_t encode_string
(
	std::string &retval,
	HuffCodeMap& codes,
	const void *data, 
	size_t data_size
)
{
	size_t r = 0;
	retval = "";

	/*
	for (int i = 0; i < data_size; ++i)
	{
		codes[data[i]];
	}
	
	for (HuffCodeMap::const_iterator it = codes.begin(); it != codes.end(); ++it)
	{
		std::cout << it->first << " ";
		std::copy(it->second.begin(), it->second.end(), std::ostream_iterator<bool>(std::cout));
		std::cout << std::endl;
	}
*/
	return r;
}

HuffCode getHuffCode
(
	const std::string &t
)
{
	HuffCode hc;
	for (int i = 0; i < t.length(); i++)
	{
		hc.push_back(t[i] == '1');
	}
	return hc;
}
