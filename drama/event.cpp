#include "event.h"
#include "../engine/moveType.h"
#include "../engine/area.h"

Block* DramaticEvent::getEntranceToArea(const Area& area, const MoveType& moveType) const
{
	if(moveType.fly)
	{
		Block* block =  selectBlockForFlyingEntrance();
		if(block)
			return block;
	}
	if(!moveType.walk)
	{
		assert(!moveType.swim.empty());
		for(auto pair : moveType.swim)
		{
			Block* block = selectBlockForSwimmingEntrance(*pair.first);
			if(block)
				return block;
		}
	}
	else
	{
		std::vector<Block*> onEdgeAtSurface = findOnEdgeAtSurface();
		Block* candidate = nullptr;
	}

}
