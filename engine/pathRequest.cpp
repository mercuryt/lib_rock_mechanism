#include "pathRequest.h"
#include "area.h"
#include "types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "items/items.h"
#include "plants.h"
// Handle
void PathRequest::create(Area& area, ActorIndex actor, DestinationCondition destination, bool detour, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	// Store paramaters for serialization.
	m_reserve = reserve;
	if(m_reserve)
		assert(m_unreserved);
	m_actor = actor;
	m_detour = detour;
	m_maxRange = maxRange;
	m_huristicDestination = huristicDestination;
	Actors& actors = area.getActors();
	TerrainFacade& terrainFacade = area.m_hasTerrainFacades.at(actors.getMoveType(actor));
	AccessCondition access = terrainFacade.makeAccessConditionForActor(actor, detour, maxRange);
	if(huristicDestination.empty())
		terrainFacade.registerPathRequestNoHuristic(actors.getLocation(actor), access, destination, *this);
	else
		terrainFacade.registerPathRequestWithHuristic(actors.getLocation(actor), access, destination, huristicDestination, *this);
}
void PathRequest::createGoTo(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_unreserved = unreserved;
	m_destination = destination;
	Blocks& blocks = area.getBlocks();
	FactionId faction = FACTION_ID_MAX;
	if(unreserved)
		faction = area.getActors().getFactionId(actor);
	// TODO: Pathing to a specific block that is reserved should do nothing but instead we iterate every block in range.
	// 	fix by adding a predicate to path request which prevents the call to findPath.
	DestinationCondition destinationCondition = [destination, &blocks, faction](BlockIndex index, Facing) {
		return index == destination && (faction == FACTION_ID_MAX || !blocks.isReserved(index, faction));
	};
	create(area, actor, destinationCondition, detour, maxRange, destination, reserve);
}
void PathRequest::createGoToAnyOf(Area& area, ActorIndex actor, std::vector<BlockIndex> destinations, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	m_destinations = destinations;
	DestinationCondition destinationCondition;
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	const Shape& shape = actors.getShape(actor);
	if(unreserved)
	{
		assert(area.getActors().getFaction(actor) != nullptr);
		FactionId faction = area.getActors().getFactionId(actor);
		std::ranges::remove_if(destinations, [&blocks, faction](BlockIndex index){ return !blocks.isReserved(index, faction); });
	}
	const MoveType& moveType = actors.getMoveType(actor);
	std::ranges::remove_if(destinations, [&blocks, &moveType](BlockIndex index){
		return blocks.shape_anythingCanEnterEver(index) && blocks.shape_moveTypeCanEnter(index, moveType);
	});
	//TODO: handle no destinations.
	assert(!destinations.empty());
	if(shape.isMultiTile)
       		destinationCondition = [&shape, &area, destinations](BlockIndex location, Facing facing) 
		{ 
			std::vector<BlockIndex> blocks = shape.getBlocksOccupiedAt(area.getBlocks(), location, facing);
			for(BlockIndex block : blocks)
				if(util::vectorContains(destinations, block))
					return true;
			return false;
		};
	else
       		destinationCondition = [destinations](BlockIndex index, Facing) { return util::vectorContains(destinations, index); };
	create(area, actor, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToLocation(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_adjacent = true;
	m_destination = destination;
	std::vector<BlockIndex> candidates;
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	for(BlockIndex block : blocks.getAdjacentWithEdgeAndCornerAdjacent(destination))
		if(
				blocks.shape_anythingCanEnterEver(block) &&
				blocks.shape_moveTypeCanEnter(block, actors.getMoveType(actor))
		)
			candidates.push_back(block);
	//TODO: handle empty candidates somehow?
	assert(!candidates.empty());
	createGoToAnyOf(area, actor, candidates, detour, unreserved, maxRange, destination, reserve);

}
void PathRequest::createGoAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	Actors& actors = area.getActors();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto otherAdjacent = actors.getAdjacentBlocks(other);
	std::vector<BlockIndex> candidates(otherAdjacent.begin(), otherAdjacent.end());
	createGoToAnyOf(area, actor, candidates, detour, unreserved, maxRange, actors.getLocation(other), reserve);
}
void PathRequest::createGoAdjacentToItem(Area& area, ActorIndex actor, ItemIndex item, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	Items& items = area.getItems();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto itemAdjacent = items.getAdjacentBlocks(item);
	std::vector<BlockIndex> candidates(itemAdjacent.begin(), itemAdjacent.end());
	createGoToAnyOf(area, actor, candidates, detour, unreserved, maxRange, items.getLocation(item), reserve);
}
void PathRequest::createGoAdjacentToPlant(Area& area, ActorIndex actor, PlantIndex plant, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	Plants& plants = area.getPlants();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto plantAdjacent = plants.getAdjacentBlocks(plant);
	std::vector<BlockIndex> candidates(plantAdjacent.begin(), plantAdjacent.end());
	createGoToAnyOf(area, actor, candidates, detour, unreserved, maxRange, plants.getLocation(plant), reserve);
}
void PathRequest::createGoAdjacentToPolymorphic(Area& area, ActorIndex actor, ActorOrItemIndex actorOrItem, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	if(actorOrItem.isActor())
		createGoAdjacentToActor(area, actor, actorOrItem.get(), detour, unreserved, maxRange, reserve);
	else
		createGoAdjacentToItem(area, actor, actorOrItem.get(), detour, unreserved, maxRange, reserve);
}
void PathRequest::createGoAdjacentToFluidType(Area& area, ActorIndex actor, const FluidType& fluidType, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_fluidType = &fluidType;
	Blocks& blocks = area.getBlocks();
	std::function<bool(BlockIndex)> condition = [&blocks, fluidType](BlockIndex index){
		return blocks.fluid_contains(index, fluidType);
	};
	createGoAdjacentToCondition(area, actor, condition, detour, unreserved, maxRange, BlockIndex::null(), reserve);
}
void PathRequest::createGoAdjacentToDesignation(Area& area, ActorIndex actor, BlockDesignation designation, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_designation = designation;
	Blocks& blocks = area.getBlocks();
	FactionId faction = area.getActors().getFactionId(actor);
	std::function<bool(BlockIndex)> condition = [&blocks, faction, designation](BlockIndex index){
		return blocks.designation_has(index, faction, designation);
	};
	createGoAdjacentToCondition(area, actor, condition, detour, unreserved, maxRange, BlockIndex::null(), reserve);
}
void PathRequest::createGoToEdge(Area& area, ActorIndex actor, bool detour)
{
	Blocks& blocks = area.getBlocks();
	std::function<bool(BlockIndex)> condition = [&blocks](BlockIndex index){ return blocks.isEdge(index); };
	createGoAdjacentToCondition(area, actor, condition, detour, false, BLOCK_DISTANCE_MAX, BlockIndex::null());
}
void PathRequest::createGoToCondition(Area& area, ActorIndex actor, DestinationCondition condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	FactionId faction = FACTION_ID_MAX;
	if(unreserved)
		faction = area.getActors().getFactionId(actor);
	Blocks& blocks = area.getBlocks();
	//TODO: use seperate lambdas rather then always passing unreserved and faction.
	DestinationCondition destinationCondition = [&blocks, condition, unreserved, faction](BlockIndex index, Facing facing) { 
		return condition(index, facing) && (!unreserved || !blocks.isReserved(index, faction));
	};
	create(area, actor, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToCondition(Area& area, ActorIndex actor, std::function<bool(BlockIndex)> condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	FactionId faction = FACTION_ID_MAX;
	if(unreserved)
	{
		faction = area.getActors().getFactionId(actor);
		assert(faction != FACTION_ID_MAX);
	}
	const Shape& shape = area.getActors().getShape(actor);
	DestinationCondition destinationCondition;
	Blocks& blocks = area.getBlocks();
	//TODO: use seperate lambdas rather then always passing unreserved and faction.
	if(shape.isMultiTile)
       		destinationCondition = [&shape, &blocks, condition, unreserved, faction](BlockIndex location, Facing facing) 
		{ 
			for(BlockIndex block : shape.getBlocksWhichWouldBeAdjacentAt(blocks, location, facing))
				if(condition(block) && (!unreserved || !blocks.isReserved(block, faction)))
					return true;
			return false;
		};
	else
       		destinationCondition = [&blocks, condition, unreserved, faction](BlockIndex index, Facing) { 
			for(BlockIndex block : blocks.getAdjacentWithEdgeAndCornerAdjacent(index))
				if(condition(block) && (!unreserved || !blocks.isReserved(block, faction)))
					return true;
			return false;
	};
	create(area, actor, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::cancel(Area& area, ActorIndex actor)
{
	TerrainFacade& terrainFacade = area.m_hasTerrainFacades.at(area.getActors().getMoveType(actor));
	if(m_huristicDestination.exists())
		terrainFacade.unregisterWithHuristic(m_index);
	else
		terrainFacade.unregisterNoHuristic(m_index);
	reset();
}
void PathRequest::reset()
{
	m_index.clear();
	m_fluidType = nullptr;
	m_actor.clear();
	m_destination = BlockIndex::null();
	m_huristicDestination.clear();
	m_maxRange = 0;
	m_designation = BlockDesignation::BLOCK_DESIGNATION_MAX;
	m_detour = false;
	m_unreserved = false;
	m_adjacent = false;
	m_reserve = false;
	m_reserveBlockThatPassesPredicate = false;
}
void PathRequest::callback(Area& area, FindPathResult& result)
{
	area.getActors().move_pathRequestCallback(m_actor, result.path, result.useCurrentPosition, m_reserve);
}
void ObjectivePathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	// No path found.
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(actor, m_objective);
		return;
	}
	if(m_reserve)
	{
		// Try to reserve blocks that will be occupied during work.
		if(result.useCurrentPosition)
		{
			if(!actors.move_tryToReserveOccupied(actor))
			{
				actors.objective_canNotCompleteSubobjective(actor);
				return;
			}
		}
		else
			if(!actors.move_tryToReserveProposedDestination(actor, result.path))
			{
				actors.objective_canNotCompleteSubobjective(actor);
				return;
			}
		// Try to reserve block which passed predicate.
		if(m_reserveBlockThatPassedPredicate && !actors.canReserve_tryToReserveLocation(actor, result.blockThatPassedPredicate))
		{
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
	}
	if(result.useCurrentPosition)
		actors.objective_execute(actor);
	else
		actors.move_setPath(actor, result.path);
	onSuccess(area, result.blockThatPassedPredicate);
}
void NeedPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	// No path found.
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotFulfillNeed(actor, m_objective);
		return;
	}
	if(result.useCurrentPosition)
		actors.objective_execute(actor);
	else
		actors.move_setPath(actor, result.path);
}
