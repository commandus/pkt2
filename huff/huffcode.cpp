#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>

#include <queue>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iterator>
#include <limits>

#include "huffcode.h"
#include "internalnode.h"
#include "leafnode.h"

#include "varint.h"
#include "bitstream.h"
#include "utilstring.h"
#include "devdecoder.h"

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
	std::istream *strm
)
{
	size_t r = 0;
	memset(frequencies, 0, sizeof(size_t) * symbols_size);
	std::string line;
	while (std::getline(*strm, line))
	{
		std::vector<int64_t> inputs;
		std::istringstream in(line);
		std::copy(std::istream_iterator<int64_t>(in), std::istream_iterator<int64_t>(), std::back_inserter(inputs));
		if (inputs.size() >= 2)
		{
			if ((inputs[0] < symbols_size) && (inputs[0] >= 0))
			{
				frequencies[inputs[0]] = inputs[1];
			}
			else
			{
				std::cerr << "frequencies file column is symbol number " << inputs[0] << " is not between 0 and " << symbols_size<< " in line: " << line << std::endl;
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
	bool include_zeroes
)
{
	for (int i = 0; i < symbols_size; ++i)
	{
		if (include_zeroes || (frequencies[i] != 0))
		{
			strm << std::dec << std::setw(3) << std::left << std::setfill(' ') << i << std::right << "\t"
				<< frequencies[i] << "\t"
				<< "0x" << std::setw(2) << std::setfill('0') << std::hex << i
				<< std::dec << std::endl;
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
			trees.push(new LeafNode(frequencies[i], (char)i, 0));
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
 
/**
 * @brief Convert huffman codes map to the Huffman tree
 * @param codes Huffman codes map
 * @return Huffman tree
 */
Node* buildTreeFromCodes
(
	const HuffCodeMap& codes
)
{
	std::priority_queue<Node*, std::vector<Node*>, NodeCmp> trees;

	int f = codes.size() + 1;
	for (std::map<unsigned char, HuffCode>::const_iterator it(codes.begin()); it != codes.end(); ++it)
	{
		trees.push(new LeafNode(f, it->first, 0));
		f--;
	}
	
	if (trees.size() == 0)
		return NULL;
	while (trees.size() > 1)
	{
		Node* childR = trees.top();
		trees.pop();

		Node* childL = trees.top();
		trees.pop();

		Node* parent = new InternalNode(childL, childR);
		trees.push(parent);
	}
	return trees.top();
}

/**
 * @param outCodes return value. Before do outCodes.clear();
 * @param node Huffman code tree
 * @param prefix First huffman code
 */
void generateCodes
(
	HuffCodeMap &outCodes,
	const Node *node, 
	const HuffCode &prefix 
)
{
	if (const LeafNode *lf = dynamic_cast<const LeafNode*>(node))
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

/**
 * @brief Return maximum depth of the Huffman tree
 * @param node Huffman tree root node
 * @return maximum depth of the Huffman tree
 * @see max_depth
 */
static int max_depth0
(
	int depth,
	const Node* node
)
{
	if (const LeafNode* lf = dynamic_cast<const LeafNode*>(node))
	{
		return depth + 1;
	}
	else
	{
		if (const InternalNode* in = dynamic_cast<const InternalNode*>(node))
		{
			depth++;
			int l = max_depth0(depth, in->left);
			int r = max_depth0(depth, in->right);
			return l > r ? l : r;
		}
	}
	return 0;
}

/**
 * @brief Return maximum depth of the Huffman tree
 * @param node Huffman tree root node
 * @return maximum depth of the Huffman tree
 */
int get_tree_max_depth
(
	const Node* node
)
{
	int d = 0;
	return max_depth0(d, node);
}

// ----------------------------------------------------------------------------------------
// https://stackoverflow.com/questions/36802354/print-binary-tree-in-a-pretty-way-using-c

struct cell_display 
{
	std::string   valstr;
	bool present;
	cell_display() 
		: present(false) {}
	cell_display(std::string valstr) 
		: valstr(valstr), present(true) {}
};

// using display_rows = std::vector< std::vector<cell_display > >;
typedef std::vector<std::vector<cell_display > > display_rows;

// The text tree generation code below is all iterative, to avoid stack faults.
// get_row_display builds a vector of vectors of cell_display structs
// each vector of cell_display structs represents one row, starting at the root
display_rows get_row_display
(
	Node *root
)
{
	// start off by traversing the tree to build a vector of vectors of Node pointers
	std::vector<Node*> traversal_stack;
	std::vector<std::vector<Node*> > rows;
	if (!root) 
		return display_rows();

	int max_depth = get_tree_max_depth(root);
	Node *p = root;
	rows.resize(max_depth);
	int depth = 0;
	for(;;) {
		// Max-depth Nodes are always a leaf or null
		// This special case blocks deeper traversal
		if (depth == max_depth - 1) 
		{
			rows[depth].push_back(p);
			if (depth == 0) 
				break;
			--depth;
			continue;
		}

		// First visit to node?  Go to left child.
		if (traversal_stack.size() == depth) {
			rows[depth].push_back(p);
			traversal_stack.push_back(p);
			if (p) 
				if (const InternalNode* n = dynamic_cast<const InternalNode*>(p))
				{
					p = n->left;
					++depth;
				}
			continue;
		}

		// Odd child count? Go to right child.
		if (rows[depth + 1].size() % 2) {
			p = traversal_stack.back();
			if (p) 
				if (const InternalNode* n = dynamic_cast<const InternalNode*>(p))
				{
					p = n->right;
					++depth;
				}
			continue;
		}

		// Time to leave if we get here
		// Exit loop if this is the root
		if (depth == 0) 
			break;

		traversal_stack.pop_back();
		p = traversal_stack.back();
		--depth;
	}

	// Use rows of Node pointers to populate rows of cell_display structs.
	// All possible slots in the tree get a cell_display struct,
	// so if there is no actual Node at a struct's location,
	// its boolean "present" field is set to false.
	// The struct also contains a string representation of
	// its Node's value, created using a std::stringstream object.
	display_rows rows_disp;
	std::stringstream ss;
	for (std::vector<std::vector<Node*> >::const_iterator it(rows.begin()); it != rows.end(); ++it)
	{
		// rows_disp.emplace_back(); // C++11 feature
		std::vector<cell_display > cd;
		rows_disp.push_back(cd); 
		for (std::vector<Node*>::const_iterator nit(it->begin()); nit != it->end(); ++nit) 
		{
			if (*nit) 
			{
				if (const InternalNode *n = dynamic_cast<const InternalNode*>(*nit))
				{
					ss << "?";
				}
				if (const LeafNode *n = dynamic_cast<const LeafNode*>(*nit))
				{
					ss << (int) n->c;
				}
				rows_disp.back().push_back(cell_display(ss.str()));
				ss.clear();
			} 
			else 
			{
				rows_disp.back().push_back(cell_display());
	}   }   }
	return rows_disp;
}

// row_formatter takes the vector of rows of cell_display structs 
// generated by get_row_display and formats it into a test representation
// as a vector of strings
std::vector<std::string> row_formatter(const display_rows &rows_disp) 
{
	typedef std::string::size_type s_t;

	// First find the maximum value string length and put it in cell_width
	s_t cell_width = 0;
	for (display_rows::const_iterator it(rows_disp.begin()); it != rows_disp.end(); ++it) 
	{
		
		for (std::vector<cell_display>::const_iterator itc(it->begin()); itc != it->end(); ++itc) 
		{
			if (itc->present && itc->valstr.length() > cell_width) 
			{
				cell_width = itc->valstr.length();
			}
		}
	}

	// make sure the cell_width is an odd number
	if (cell_width % 2 == 0) 
		++cell_width;

	// formatted_rows will hold the results
	std::vector<std::string> formatted_rows;

	// some of these counting variables are related,
	// so its should be possible to eliminate some of them.
	s_t row_count = rows_disp.size();

	// this row's element count, a power of two
	s_t row_elem_count = 1 << (row_count - 1);

	// left_pad holds the number of space charactes at the beginning of the bottom row
	s_t left_pad = 0;

	// Work from the level of maximum depth, up to the root
	// ("formatted_rows" will need to be reversed when done) 
std::cerr << "row_formatter " << row_count << std::endl;		

	
	for (s_t r = 0; r < row_count; ++r) 
	{
std::cerr << "row: " << r << " of " << row_count << " elements: " << row_elem_count << std::endl;		

		const std::vector<cell_display > &cd_row = rows_disp[row_count - r - 1]; // r reverse-indexes the row
		// "space" will be the number of rows of slashes needed to get
		// from this row to the next.  It is also used to determine other
		// text offsets.
		s_t space = (s_t(1) << r) * (cell_width + 1) / 2 - 1;
		// "row" holds the line of text currently being assembled
		std::stringstream ss;
		// iterate over each element in this row
		for (s_t c = 0; c < row_elem_count; ++c) 
		{
			// add padding, more when this is not the leftmost element
			ss << std::string(c ? left_pad * 2 + 1 : left_pad, ' ');
			if ((c < cd_row.size()) && cd_row[c].present) 
			{
				// This position corresponds to an existing Node
				const std::string &valstr = cd_row[c].valstr;
				// Try to pad the left and right sides of the value string
				// with the same number of spaces.  If padding requires an
				// odd number of spaces, right-sided children get the longer
				// padding on the right side, while left-sided children
				// get it on the left side.
				s_t padding;
				s_t long_padding = cell_width - valstr.length();
				s_t short_padding = long_padding / 2;
				if (c % 2)
					padding = short_padding;
				else
					padding = long_padding;
				if (padding > 0)
					ss << std::string(padding, ' ');
				ss << valstr;
				if (padding > 0)
					ss << std::string(padding, ' ');
			}
			else 
			{
				// This position is empty, Nodeless...
				ss << std::string(cell_width, ' ');
			}
		}
		// A row of spaced-apart value strings is ready, add it to the result vector
		formatted_rows.push_back(ss.str());

		// The root has been added, so this loop is finsished
		if (row_elem_count == 1) 
			break;

		// Add rows of forward- and back- slash characters, spaced apart
		// to "connect" two rows' Node value strings.
		// The "space" variable counts the number of rows needed here.
		s_t left_space  = space + 1;
		s_t right_space = space - 1;
		for (s_t sr = 0; sr < space; ++sr) 
		{
			std::string row;
			for (s_t c = 0; c < row_elem_count; ++c) 
			{
				if (c % 2 == 0) 
				{
					row += std::string(c ? left_space * 2 + 1 : left_space, ' ');
					row += cd_row[c].present ? '/' : ' ';
					row += std::string(right_space + 1, ' ');
				} else {
					row += std::string(right_space, ' ');
					row += cd_row[c].present ? '\\' : ' ';
				}
			}
			formatted_rows.push_back(row);
			++left_space;
			--right_space;
		}
		left_pad += space + 1;
		row_elem_count /= 2;
	}
	// Reverse the result, placing the root node at the beginning (top)
	std::reverse(formatted_rows.begin(), formatted_rows.end());
	return formatted_rows;
}

// Trims an equal number of space characters from
// the beginning of each string in the vector.
// At least one string in the vector will end up beginning
// with no space characters.
static void trim_rows_left(std::vector<std::string> &rows) 
{
	if (!rows.size()) 
		return;
	size_t min_space = rows.front().length();
	for (std::vector<std::string>::const_iterator it(rows.begin()); it != rows.end(); ++it) 
	{
		size_t i = it->find_first_not_of(' ');
		if (i == std::string::npos) 
			i = it->length();
		if (i == 0) 
			return;
		if (i < min_space) 
			min_space = i;
	}
	
	for (std::vector<std::string>::iterator it(rows.begin()); it != rows.end(); ++it) 
	{
		it->erase(0, min_space);
}   }

/**
 * @brief Dumps a representation of the tree to cout
 */
void dump_tree
(
	Node *node
) 
{
	const int d = get_tree_max_depth(node);

	// If this tree is empty, tell someone
	if (d == 0) 
	{
		std::cout << " <empty tree>" << std::endl;
		return;
	}

	// This tree is not empty, so get a list of node values...
	const display_rows rows_disp = get_row_display(node);
	// then format these into a text representation...
	std::vector<std::string> formatted_rows = row_formatter(rows_disp);
	// then trim excess space characters from the left sides of the text...
	trim_rows_left(formatted_rows);
	// then dump the text to cout.
	
	for (std::vector<std::string>::const_iterator it(formatted_rows.begin()); it != formatted_rows.end(); ++it) 
	{
		std::cout << ' ' << *it << std::endl;
	}
}

// ----------------------------------------------------------------------------------------

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
)
{
	
	std::stringstream ss;
	if (const LeafNode* lf = dynamic_cast<const LeafNode*>(node))
	{
		strm 
			<< std::dec << std::setw(3) << std::left << std::setfill(' ') << (int) lf->c
			<< " 0x"
			<< std::hex << std::setw(2) << std::left << std::setfill(' ') << (int) lf->c
		<< std::endl;
	}
	else
	{
		if (const InternalNode* in = dynamic_cast<const InternalNode*>(node))
		{
			// strm << std::dec << level << ") Left" << std::endl;
			
			HuffCode leftPrefix = prefix;
			leftPrefix.push_back(false);
			print_tree(level + 1, strm, in->left, leftPrefix);

			// strm << std::dec << level << ") Right" << std::endl;

			HuffCode rightPrefix = prefix;
			rightPrefix.push_back(true);
			print_tree(level + 1, strm, in->right, rightPrefix);
		}
	}

}

void printCodeMap
(
	std::ostream &strm,
	const HuffCodeMap& codes
)
{
	for (HuffCodeMap::const_iterator it = codes.begin(); it != codes.end(); ++it)
	{
		strm << std::dec << std::setw(3) << std::left << std::setfill(' ') << (int) it->first << "\t" << std::right;
		std::copy(it->second.begin(), it->second.end(), std::ostream_iterator<bool>(std::cerr));
		strm << "\t0x" << std::setw(2) << std::setfill('0') << std::hex << (int) it->first;
		strm << std::right << std::dec << std::endl;
	}
}

/**
 * Read Huffman code from the binary text representaion
 * @param t binary text representation
 */
HuffCode getHuffCode(const std::string &t)
{
	HuffCode hc;
	for (int i = 0; i < t.length(); i++)
	{
		hc.push_back(t[i] == '1');
	}
	return hc;
}

// https:stackoverflow.com/questions/8362094/replace-multiple-spaces-with-one-space-in-a-string
static bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }

size_t loadCodeMap
(
	HuffCodeMap& codes,
	std::istream *strm
)
{
	codes.clear();
	size_t r = 0;
	std::string line;
	while (std::getline(*strm, line))
	{
		std::replace(line.begin(), line.end(), '\t', ' ');
		std::string::iterator new_end = std::unique(line.begin(), line.end(), BothAreSpaces);
		line.erase(new_end, line.end());

		std::vector<std::string> t = pkt2utilstring::split(line, ' ');
		if (t.size() < 2)
			continue;
		uint64_t symbol = strtod(t[0].c_str(), NULL);
		if (symbol <= 255)
		{
			codes[symbol] = getHuffCode(t[1]);
		}
		else
		{
			std::cerr << "Map file first column is symbol number " << symbol << " is not between 0 and " << 255 << " in line " << r + 1 << ": " << line << std::endl;
			return r;
		}
		r++;
	}
	return r;
}

static unsigned char bitsRequired
(
	unsigned char *value
)
{
	unsigned char r = 0;
	while (*value >>= 1)
		r++;
	return r;
}

static const HuffCodeNSize *findHuffCodeNSize
(
	const HuffCodeNSizes &escape_code_sizes,
	const HuffCode &value
)
{
	const HuffCodeNSize *r = NULL;
	for (int i = 0; i < escape_code_sizes.size(); i++)
	{
		if (escape_code_sizes.at(i).first == value)
		{
			r = &escape_code_sizes[i];
			break;
		}
	}
	return r;
}

static const HuffCodeNSize *findShortestHuffCodeNSize
(
	const HuffCodeNSizes &escape_code_sizes,
	unsigned char *value
)
{
	const HuffCodeNSize *r = NULL;
	unsigned char sz = bitsRequired(value);
	unsigned int dmin = sizeof(value);
	int idx = -1;
	
	for (int i = 0; i < escape_code_sizes.size(); i++)
	{
		int d = escape_code_sizes[i].second - sz;
		if (d < 0)
			continue;
		if (d < dmin)
		{
			idx = i;
			dmin = d;
		}
	}
	if (idx >= 0)
		r = &escape_code_sizes[idx];
	return r;
}

/**
 * @param retval compressed stream
 * @param codes Huffman codes
 * @param escape_code_sizes Huffman code and size
 * @param eof_code Huffman code (can be NULL- no write EOF)
 * @param data buffer
 * @param size buffer size 
 * @return bytes
 */
static size_t compress_buffer
(
	std::ostream *retval,
	const HuffCodeMap& codes,
 	const HuffCodeNSizes &escape_code_sizes,
	const HuffCode *eof_code,  
	const void *data, 
	size_t size
)
{
	size_t r = 0;
	obitstream strm(retval);
	
	for (int i = 0; i < size; ++i)
	{
		std::map<unsigned char, HuffCode>::const_iterator it = codes.find(((unsigned char*) data)[i]);
		if (it == codes.end())
		{
			const HuffCodeNSize *hcs = findShortestHuffCodeNSize(escape_code_sizes, ((unsigned char*) data) + i);
			if (!hcs)
			{
				std::cerr << "Fatal error: no code for " << (int)((unsigned char*) data)[i] 
					<< " 0x" << std::hex << std::setw(2) << (int)((unsigned char*) data)[i] 
					<< std::dec 
					<< " and no escape full byte Huffman code assigned"
					<< std::endl;
				return std::numeric_limits<uint64_t>::max();
			}
			else
			{
				// write escape code & value itself
				strm.write(hcs->first);
				strm.write((int)((unsigned char*) data)[i], hcs->second);
				r += hcs->first.size() + hcs->second;
			}
		}
		else
		{
			const HuffCodeNSize *hcs = findHuffCodeNSize(escape_code_sizes, it->second);
			if (hcs)
			{
				// write escape code & value itself
				strm.write(hcs->first);
				strm.write((int)((unsigned char*) data)[i], hcs->second);
				r += hcs->first.size() + hcs->second;
			}
			else
			{
				strm.write(it->second);
				r += it->second.size();
			}
		}
	}
	if (eof_code)
	{
		strm.write(*eof_code);
		r += eof_code->size();
	}
	return r;
}

/**
 * @param codes Huffman codes
 * @param escape_code_sizes Huffman code
 * @param eof_code Huffman code
 * @param data buffer
 * @param size buffer size 
 * @return bytes
 */
static size_t compress_buffer_calc_only
(
	const HuffCodeMap& codes,
 	const HuffCodeNSizes &escape_code_sizes,
	const HuffCode *eof_code,   
	const void *data, 
	size_t size
)
{
	size_t r = 0;
	
	for (int i = 0; i < size; ++i)
	{
		std::map<unsigned char, HuffCode>::const_iterator it = codes.find(((unsigned char*) data)[i]);
		if (it == codes.end())
		{
			const HuffCodeNSize *hcs = findShortestHuffCodeNSize(escape_code_sizes, ((unsigned char*) data) + i);
			if (!hcs)
			{
				std::cerr << "Fatal error: no code for " << (int)((unsigned char*) data)[i] 
					<< " 0x" << std::hex << std::setw(2) << (int)((unsigned char*) data)[i] 
					<< std::dec 
					<< " and no escape full byte Huffman code assigned"
					<< std::endl;
				return std::numeric_limits<uint64_t>::max();
			}
			else
			{
				// write escape code & value itself
				r += hcs->first.size() + hcs->second;
			}
		}
		else
		{
			const HuffCodeNSize *hcs = findHuffCodeNSize(escape_code_sizes, it->second);
			if (hcs)
			{
				// write escape code & value itself
				r += hcs->first.size() + 8;
			}
			else
			{
				r += it->second.size();
			}
		}
	}
	if (eof_code)
	{
		r += eof_code->size();
	}
	return r;
}

/**
 * @brief Find out leaf node by the code
 * @return NULL if not found
 */
static LeafNode *find_code
(
	const Node *root,
	const HuffCode &code
)
{
	Node *n = (Node*) root;
	for (int i = 0; i < code.size(); i++)
	{
		if (!code.at(i))
		{
			// to right
			if (const InternalNode* in = dynamic_cast<const InternalNode*>(n)) 
			{
				// n = in->right;
				n = in->left;
			}
		}
		else
		{
			// to left
			if (const InternalNode* in = dynamic_cast<const InternalNode*>(n)) 
			{
				// n = in->left;
				n = in->right;
			}
		}	
	}
		
	if (LeafNode *lf = dynamic_cast<LeafNode*>(n))
	{
		return lf;
	}
	return NULL;
}

/**
 * @brief Find out leaf node by the code and set flags
 * @return NULL if not found
 */
static bool set_code_flag
(
	const Node *root,
	const HuffCode &code,
	uint8_t value
)
{
	LeafNode *r = find_code(root, code);
	if (!r)
		return false;
	r->flags |= value;
	return true;
}

/**
 * @brief Find out leaf node by the code and clear flags
 * @return NULL if not found
 */
static bool clear_code_flag
(
	const Node *root,
	const HuffCode &code
)
{
	LeafNode *r = find_code(root, code);
	if (!r)
		return false;
	r->flags = 0;
	return true;
}

/**
 * @param mode 0- never used, 1- slower, 2- faster
 * @param retval decompressed output stream
 * @param root Huffman codes tree root node
 * @param escape_code_sizes Huffman code and size
 * @param input_stream input stream
 * @param decompressed_size original size
 * @return bytes
 */
static size_t decompress_stream
(
	int mode, 
	std::ostream *retval,
	const Node *root,
	const HuffCodeNSizes &escape_code_sizes, 
	const HuffCode *eof_code, 
	std::istream *input_stream, 
	size_t decompressed_size
)
{
	if (mode == 2)
	{
		char b_out[80];
		std::istreambuf_iterator<char> eos;
		std::string s(std::istreambuf_iterator<char>(*input_stream), eos);
		std::string ss = "12" + s;
		size_t sz = dec_hafman(b_out, (unsigned char *) ss.c_str(), decompressed_size + 2);
		if (sz > 2) 
		{
			retval->write(b_out, sz - 2);
		}
	}

	for (int i = 0; i < escape_code_sizes.size(); i++)
	{
		if (escape_code_sizes.at(i).first.size())
			set_code_flag(root, escape_code_sizes.at(i).first, 2);
	}
	if (eof_code)
		set_code_flag(root, *eof_code, 1);
	
	size_t r = 0;
	ibitstream strm(input_stream);
	int bit;
	
	Node *n = (Node*) root;
	
	while (true)
	{
		bit = strm.read();
		if (n)
		{
			switch (bit) {
				case -1:
					// EOF
					n = NULL;
					break;
				case 0:
					// to left
					if (const InternalNode* in = dynamic_cast<const InternalNode*>(n)) 
					{
						n = in->left;
					}
					break;
				case 1:
					// to right
					if (const InternalNode* in = dynamic_cast<const InternalNode*>(n)) 
					{
						n = in->right;
					}
					break;
				default:
					break;
			}
		}

		// check is it time to append code
		if (const LeafNode *lf = dynamic_cast<const LeafNode*>(n))
		{
			r++;
			// check size if provided
			if (!n || ((eof_code == NULL) && (r >= decompressed_size)))
			{
				// all done, last bits are garbage!
				break;
			}
			
			if (lf->flags & 1)
			{
				// eof
				break;
			}
			if (lf->flags & 2)
			{
				// get next 8 bits entirely as escaped code
				char escaped_byte = strm.read8();
				// write byte
				if (retval)
					*retval << escaped_byte;
			}
			else
			{
				// write byte
				if (retval)
					*retval << lf->c;
			}
			
			// reset, let read next code
			n = (Node*) root;
		}
	}
	return r;
}


/**
 * @param mode 0- never used, 1- slower, 2- faster
 * @param retval decompressed output stream
 * @param root Huffman codes tree root node
 * @param escape_code_sizes Huffman code
 * @param data buffer
 * @param size buffer size 
 * @param decompressed_size original size
 * @return bytes
 */
static size_t decompress_buffer
(
	int mode,
	std::ostream *retval,
	const Node *root,
 	const HuffCodeNSizes &escape_code_sizes, 
	const HuffCode *eof_code, 
	const void *data, 
	size_t size,
	size_t decompressed_size
)
{
	std::stringstream ss(std::string((char *) data, size));
	return decompress_stream(mode, retval, root, escape_code_sizes, eof_code, &ss, decompressed_size);
}

/**
 * Write size as variable length integer
 * Other options are optional
 */
size_t write_header
(
	std::ostream *retval,
	size_t size
)
{
	return obitstream::write_varint(retval, size);
}

/**
 * Read size as variable length integer
 */
size_t read_header(
	std::istream *retval
)
{
	return ibitstream::read_varint(retval);
}

/**
 * @param retval compressed stream
 * @param codes Huffman codes
 * @param escape_code Huffman code
 * @param eof_code NULL- write length descriptor, otherwise write EOF symbol
 * @param compression_offset offset in data buffer
 * @param data buffer
 * @param size buffer size 
 * @return bytes
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
)
{
	size_t r = 0;
	if (!eof_code)
	{
		if (retval)
			r += write_header(retval, size);
		else
		{
			// just cals size
			uint8_t buf[4];
			r += encodeVarint<uint64_t>(size, (uint8_t*) &buf);
		}
	}

	if (compression_offset > 0)
	{
		if (retval)
		{
		for (int i = 0; i < compression_offset; i++)
			*retval << ((char *) data) [i];
		}
		if (retval)
			r += compression_offset + compress_buffer(retval, codes, escape_code_sizes, eof_code, &((char *) data) [compression_offset], size - compression_offset);
		else
			r += compression_offset + compress_buffer_calc_only(codes, escape_code_sizes, eof_code, &((char *) data) [compression_offset], size - compression_offset);
	}
	else
	{
		if (retval)
			r += compress_buffer(retval, codes, escape_code_sizes, eof_code, data, size);
		else
			r += compress_buffer_calc_only(codes, escape_code_sizes, eof_code, data, size);
	}
	return r;
}

/**
 * @param mode 0- never used, 1- slower, 2- faster
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
)
{
	// get sz: original size in bytes and
	// get length_descriptor_size: bytes occupied by length descriptor size in bytes(variable length integer)
	uint64_t sz;
	size_t length_descriptor_size;
	if (force_size > 0) 
	{
		length_descriptor_size = 0;
		sz = force_size;
	}
	else
	{
		if (eof_code)
		{
			length_descriptor_size = 0;
			sz = 0;
		}
		else
			sz = ibitstream::get_varint((uint8_t *) data, &length_descriptor_size);
	}
	
	// get data pointer
	const void *p = (char *) data + length_descriptor_size;
	if (compression_offset > 0)
	{
		if (retval)
		{
			for (int i = 0; i < compression_offset; i++)
			{
				*retval << ((char *) p) [i];
			}
		}
		return compression_offset + decompress_buffer(mode, retval, root, escape_code_sizes, eof_code, &((char *) p) [compression_offset], size - length_descriptor_size - compression_offset, sz);
	}
	else
		return decompress_buffer(mode, retval, root, escape_code_sizes, eof_code, p, size - length_descriptor_size, sz);
}

/**
 * @param mode 0- never used, 1- slower, 2- faster
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
	int mode,
	std::ostream *retval,
	HuffCodeMap& codes,
	const HuffCodeNSizes &escape_code_sizes,
	const HuffCode *eof_code,  
	int force_size,
	int compression_offset,
	const void *data, 
	size_t size
)
{
	Node *root = buildTreeFromCodes(codes);
	size_t sz = decompress(mode, retval, root, escape_code_sizes, eof_code, force_size, compression_offset, data, size);
	delete root;
	return sz;
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

Node* defaultHuffmanCodeTree
(
)
{
	std::stringstream alphabet;
	size_t frequencies[256];
	
	// Linear
	for (int i = 0; i < 256; i++) 
	{
		alphabet << (unsigned char) (256 - i);
	}
	
	// TODO logarithmic?
	
	Node* r = buildTree(frequencies, 256);
	return r;
}

HuffCodeMap defaultHuffmanCodeMap()
{
	Node *n = defaultHuffmanCodeTree();
	HuffCode hc;
	HuffCodeMap r;
	generateCodes(r, n, hc);
	return r;
}

Node* loadHuffmanCodeTreeFromFrequencyStream
(
	std::istream *frequencies_stream	///< stream
)
{
	size_t frequencies[256];
	loadFrequencies(frequencies, 256, frequencies_stream);
	Node* r = buildTree(frequencies, 256);
	return r;
}

Node* loadHuffmanCodeTreeFromFrequencyFile
(
	const std::string &frequencies_file	///< file name
)
{
	std::istream *strm = new std::ifstream(frequencies_file.c_str(), std::ifstream::in);
	Node* r = loadHuffmanCodeTreeFromFrequencyStream(strm);
	delete strm;
	return r;
}

Node* loadHuffmanCodeTreeFromCodeStream
(
	std::istream *codes_stream	///< stream
)
{
	HuffCodeMap codeMap;
	loadCodeMap(codeMap, codes_stream);
	Node *r = buildTreeFromCodes(codeMap);
	return r;
}

Node* loadHuffmanCodeTreeFromCodeFile
(
	const std::string &codes_file	///< file name
)
{
	std::istream *strm = new std::ifstream(codes_file.c_str(), std::ifstream::in);
	Node* r = loadHuffmanCodeTreeFromCodeStream(strm);
	delete strm;
	return r;
}

bool checkTags
(
	const void *data, 
	size_t size, 
	const std::vector<std::pair<int, int> > &tags)
{
	for (int i = 0; i < tags.size(); i++) 
	{
		int idx = tags[i].first;
		if ((idx < 0) || (idx >= size))
		{
			return false;
		}
		if (tags[i].second != ((uint8_t*) data)[idx])
		{
			return false;
		}
	}
	return true;
}
