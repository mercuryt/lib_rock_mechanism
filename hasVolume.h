/*
 * Something which takes up space. May require any percentage per block from any pattern of blocks.
 */

#include "hasPosition.h"
#include "shape.h"
#include "block.h"

class HasVolume : public HasPosition
{
	Shape* shape;
	bool canMoveTo(Block* block);
	void moveTo(Block* block);
}
