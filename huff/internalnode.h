#ifndef INTERNALNODE_H
#define INTERNALNODE_H

#include "node.h"

class InternalNode : public Node
{
public:
	Node *const left;
	Node *const right;
	InternalNode(Node* c0, Node* c1);
	~InternalNode();
};

#endif // INTERNALNODE_H
