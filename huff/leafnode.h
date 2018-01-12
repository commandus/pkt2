
#ifndef LEAFNODE_H
#define LEAFNODE_H

#include <inttypes.h>
#include "node.h"

class LeafNode : public Node
{
public:
	const char c;
	uint8_t flags;								// 1- eof(no char), 2- escape(entire char followed)
	LeafNode(int f, char c, uint8_t flags);
};

#endif // LEAFNODE_H
