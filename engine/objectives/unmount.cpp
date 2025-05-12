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
	const ShapeId& shape = actors.getCompoundShape(actor);
	const BlockIndex& startingLocation = actors.getLocation(actor);
	if(m_location.empty())
	{
		for(const BlockIndex& block : actors.getAdjacentBlocks(mount))
		{
			if(blocks.shape_canEnterCurrentlyFrom(m_location, shape, startingLocation, actors.getBlocks(actor)))
			{
				actors.mount_undo(actor, block, blocks.facingToSetWhenEnteringFrom(block, actors.getLocation(actor)));
				actors.objective_complete(actor, *this);
				return;
			}
		}
		actors.objective_canNotCompleteObjective(actor, *this);
	}
	else
	{
		const BlockIndex& actorLocation = actors.getLocation(actor);
		const ShapeId& compoundShape = actors.getCompoundShape(mount);
		auto currentlyAdjacent = Shape::getBlocksWhichWouldBeAdjacentAt(compoundShape, blocks, actorLocation, actors.getFacing(mount));
		if(currentlyAdjacent.contains(m_location))
		{

			const Facing4 facing = blocks.shape_canEnterEverWithAnyFacingReturnFacing(m_location, actors.getShape(actor), actors.getMoveType(actor));
			if(facing != Facing4::Null)
			{
				actors.mount_undo(actor, m_location, blocks.facingToSetWhenEnteringFrom(m_location, actorLocation));
				actors.objective_complete(actor, *this);
			}
			else
				actors.objective_canNotCompleteObjective(actor, *this);
		}
		else
			actors.move_setDestinationAdjacentToLocation(actor, m_location);
	}
}