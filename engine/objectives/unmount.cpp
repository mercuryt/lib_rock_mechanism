 #include "unmount.h"
 #include "../actors/actors.h"
 #include "../area/area.h"
 #include "../blocks/blocks.h"
UnmountObjective::UnmountObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_location(data["location"].get<BlockIndex>())
{ }
Json UnmountObjective::toJson() const
{
	Json data = Objective::toJson();
	data["location"] = m_location;
	return data;
}
void UnmountObjective::execute(Area& area, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	assert(actors.onDeck_getIsOnDeckOf(actor).isActor());
	const ActorIndex& mount = actors.onDeck_getIsOnDeckOf(actor).getActor();
	if(m_location.empty())
	{
		for(const BlockIndex& block : actors.getAdjacentBlocks(mount))
			if(blocks.shape_canEnterCurrentlyFrom(block, actors.getShape(actor), actors.getLocation(actor), actors.getBlocks(actor)))
				actors.mount_undo(actor, block);
	}
	else
	{
		const ShapeId& compoundShape = actors.getCompoundShape(mount);
		auto currentlyAdjacent = Shape::getBlocksWhichWouldBeAdjacentAt(compoundShape, blocks, actors.getLocation(mount), actors.getFacing(actor));
		if(currentlyAdjacent.contains(m_location))
		{
			if(blocks.shape_canEnterCurrentlyFrom(m_location, actors.getShape(actor), actors.getLocation(actor), actors.getBlocks(actor)))
			{
				actors.mount_undo(actor, m_location);
				actors.objective_complete(actor, *this);
			}
			else
				actors.objective_canNotCompleteObjective(actor, *this);
		}
		else
			actors.move_setDestinationAdjacentToLocation(actor, m_location);
	}
}