#include "hasShape.h"
bool isAdjacentTo(HasShape& other) const
{
	assert(this != &other);
	for(Block* block : m_blocks)
	{
		if(other.m_blocks.contains(block))
			return true;
		for(Block* adjacent : block->m_adjacentsVector)
			if(other.m_blocks.contains(adjacent))
				return true;
	}
	return false;
}
