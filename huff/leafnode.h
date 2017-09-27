
#ifndef LEAFNODE_H
#define LEAFNODE_H

#include "node.h"

class LeafNode : public Node
{
public:
	const char c;
	LeafNode(int f, char c);
};

#endif // LEAFNODE_H
