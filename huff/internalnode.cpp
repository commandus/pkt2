#include "internalnode.h"

InternalNode::InternalNode
(
	Node* c0, 
	Node* c1
)
	: Node(c0->f + c1->f), left(c0), right(c1) 
{
	
}

InternalNode::~InternalNode()
{
	delete left;
	delete right;
}
