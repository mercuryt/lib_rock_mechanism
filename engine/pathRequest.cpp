#include "pathRequest.h"
#include "area.h"
#include "deserializationMemo.h"
#include "types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "items/items.h"
#include "plants.h"
#include "objectives/dig.h"
#include "objectives/construct.h"
#include "objectives/getToSafeTemperature.h"
#include "objectives/eat.h"
#include "objectives/installItem.h"
#include "objectives/wander.h"
#include "objectives/harvest.h"
#include "objectives/drink.h"
#include "objectives/givePlantsFluid.h"
#include "objectives/sleep.h"
#include "objectives/stockpile.h"
#include "objectives/leaveArea.h"
#include "objectives/uniform.h"
// Handle
void PathRequest::create(Area& area, ActorIndex actor, DestinationCondition destination, bool detour, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	// Store paramaters for serialization.
	m_reserve = reserve;
	if(m_reserve)
		assert(m_unreserved);
	assert(actor.exists());
	m_actor = actor;
	m_detour = detour;
	m_maxRange = maxRange;
	m_huristicDestination = huristicDestination;
	Actors& actors = area.getActors();
	TerrainFacade& terrainFacade = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(actor));
	AccessCondition access = terrainFacade.makeAccessConditionForActor(actor, detour, maxRange);
	if(huristicDestination.empty())
		terrainFacade.registerPathRequestNoHuristic(actors.getLocation(actor), access, destination, *this);
	else
		terrainFacade.registerPathRequestWithHuristic(actors.getLocation(actor), access, destination, huristicDestination, *this);
	assert(m_index.exists());
}
void PathRequest::createGoTo(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_unreserved = unreserved;
	m_destination = destination;
	Blocks& blocks = area.getBlocks();
	FactionId faction;
	if(unreserved)
		faction = area.getActors().getFactionId(actor);
	// TODO: Pathing to a specific block that is reserved should do nothing but instead we iterate every block in range.
	// 	fix by adding a predicate to path request which prevents the call to findPath.
	DestinationCondition destinationCondition = [destination, &blocks, faction](BlockIndex index, Facing) {
		bool result = index == destination && (faction.empty() || !blocks.isReserved(index, faction));
		return std::make_pair(result, index);
	};
	create(area, actor, destinationCondition, detour, maxRange, destination, reserve);
}
void PathRequest::createGoToAnyOf(Area& area, ActorIndex actor, BlockIndices destinations, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	m_destinations = destinations;
	m_unreserved = unreserved;
	DestinationCondition destinationCondition;
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	ShapeId shape = actors.getShape(actor);
	if(unreserved)
	{
		assert(area.getActors().getFaction(actor).exists());
		FactionId faction = area.getActors().getFactionId(actor);
		destinations.remove_if([&blocks, faction](BlockIndex index){ return !blocks.isReserved(index, faction); });
	}
	MoveTypeId moveType = actors.getMoveType(actor);
	destinations.remove_if([&blocks, &moveType](BlockIndex index){
		return !blocks.shape_anythingCanEnterEver(index) || !blocks.shape_moveTypeCanEnter(index, moveType);
	});
	//TODO: handle no destinations.
	assert(!destinations.empty());
	if(Shape::getIsMultiTile(shape))
		destinationCondition = [shape, &area, destinations](BlockIndex location, Facing facing) 
		{
			BlockIndices blocks = Shape::getBlocksOccupiedAt(shape, area.getBlocks(), location, facing);
			for(BlockIndex block : blocks)
				if(destinations.contains(block))
					return std::make_pair(true, location);
			return std::make_pair(false, BlockIndex::null());
		};
	else
		destinationCondition = [destinations](BlockIndex index, Facing) { 
				if(destinations.contains(index))
					return std::make_pair(true, index);
				return std::make_pair(false, BlockIndex::null());
			};
	create(area, actor, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToLocation(Area& area, ActorIndex actor, BlockIndex destination, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_adjacent = true;
	m_destination = destination;
	BlockIndices candidates;
	Blocks& blocks = area.getBlocks();
	for(BlockIndex block : blocks.getAdjacentWithEdgeAndCornerAdjacent(destination))
		candidates.add(block);
	createGoToAnyOf(area, actor, candidates, detour, unreserved, maxRange, destination, reserve);

}
void PathRequest::createGoAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	Actors& actors = area.getActors();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto otherAdjacent = actors.getAdjacentBlocks(other);
	createGoToAnyOf(area, actor, otherAdjacent, detour, unreserved, maxRange, actors.getLocation(other), reserve);
}
void PathRequest::createGoAdjacentToItem(Area& area, ActorIndex actor, ItemIndex item, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	Items& items = area.getItems();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto itemAdjacent = items.getAdjacentBlocks(item);
	createGoToAnyOf(area, actor, itemAdjacent, detour, unreserved, maxRange, items.getLocation(item), reserve);
}
void PathRequest::createGoAdjacentToPlant(Area& area, ActorIndex actor, PlantIndex plant, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	Plants& plants = area.getPlants();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto plantAdjacent = plants.getAdjacentBlocks(plant);
	createGoToAnyOf(area, actor, plantAdjacent, detour, unreserved, maxRange, plants.getLocation(plant), reserve);
}
void PathRequest::createGoAdjacentToPolymorphic(Area& area, ActorIndex actor, ActorOrItemIndex actorOrItem, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	if(actorOrItem.isActor())
		createGoAdjacentToActor(area, actor, actorOrItem.getActor(), detour, unreserved, maxRange, reserve);
	else
		createGoAdjacentToItem(area, actor, actorOrItem.getItem(), detour, unreserved, maxRange, reserve);
}
void PathRequest::createGoAdjacentToFluidType(Area& area, ActorIndex actor, FluidTypeId fluidType, bool detour, bool unreserved, DistanceInBlocks maxRange, bool reserve)
{
	m_fluidType = fluidType;
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
	createGoAdjacentToCondition(area, actor, condition, detour, false, DistanceInBlocks::null(), BlockIndex::null());
}
void PathRequest::createGoToCondition(Area& area, ActorIndex actor, DestinationCondition condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	FactionId faction;
	if(unreserved)
		faction = area.getActors().getFactionId(actor);
	Blocks& blocks = area.getBlocks();
	//TODO: use seperate lambdas rather then always passing unreserved and faction.
	DestinationCondition destinationCondition = [&blocks, condition, unreserved, faction](BlockIndex index, Facing facing) { 
		bool result = condition(index, facing).first && (!unreserved || !blocks.isReserved(index, faction));
		return std::make_pair(result, index);
	};
	create(area, actor, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToCondition(Area& area, ActorIndex actor, std::function<bool(BlockIndex)> condition, bool detour, bool unreserved, DistanceInBlocks maxRange, BlockIndex huristicDestination, bool reserve)
{
	FactionId faction;
	if(unreserved)
	{
		faction = area.getActors().getFactionId(actor);
		assert(faction.exists());
		m_unreserved = true;
	}
	ShapeId shape = area.getActors().getShape(actor);
	DestinationCondition destinationCondition;
	Blocks& blocks = area.getBlocks();
	//TODO: use seperate lambdas rather then always passing unreserved and faction.
	if(Shape::getIsMultiTile(shape))
       		destinationCondition = [shape, &blocks, condition, unreserved, faction](BlockIndex location, Facing facing) 
		{ 
			for(BlockIndex block : Shape::getBlocksWhichWouldBeAdjacentAt(shape, blocks, location, facing))
				if(condition(block) && (!unreserved || !blocks.isReserved(block, faction)))
					return std::make_pair(true, block);
			return std::make_pair(false, BlockIndex::null());
		};
	else
       		destinationCondition = [&blocks, condition, unreserved, faction](BlockIndex index, Facing) { 
			for(BlockIndex block : blocks.getAdjacentWithEdgeAndCornerAdjacent(index))
				if(condition(block) && (!unreserved || !blocks.isReserved(block, faction)))
					return std::make_pair(true, block);
			return std::make_pair(false, BlockIndex::null());
	};
	create(area, actor, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::cancel(Area& area, ActorIndex actor)
{
	TerrainFacade& terrainFacade = area.m_hasTerrainFacades.getForMoveType(area.getActors().getMoveType(actor));
	if(m_huristicDestination.exists())
		terrainFacade.unregisterWithHuristic(m_index);
	else
		terrainFacade.unregisterNoHuristic(m_index);
	reset();
}
void PathRequest::reset()
{
	m_index.clear();
	m_fluidType.clear();
	m_actor.clear();
	m_destination = BlockIndex::null();
	m_huristicDestination.clear();
	m_maxRange = DistanceInBlocks::create(0);
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
std::unique_ptr<PathRequest> PathRequest::load(Area& area, const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data["type"] == "basic")
		return std::make_unique<PathRequest>(data);
	if(data["type"] == "attack")
		return std::make_unique<GetIntoAttackPositionPathRequest>(area, data);
	if(data["type"] == "dig")
		return std::make_unique<DigPathRequest>(data, deserializationMemo);
	if(data["type"] == "temperature")
		return std::make_unique<GetToSafeTemperaturePathRequest>(data, deserializationMemo);
	if(data["type"] == "construct")
		return std::make_unique<ConstructPathRequest>(data, deserializationMemo);
	if(data["type"] == "eat")
		return std::make_unique<EatPathRequest>(data, deserializationMemo);
	if(data["type"] == "craft")
		return std::make_unique<CraftPathRequest>(data, deserializationMemo);
	if(data["type"] == "install item")
		return std::make_unique<InstallItemPathRequest>(data, deserializationMemo);
	if(data["type"] == "wander")
		return std::make_unique<WanderPathRequest>(data, deserializationMemo);
	if(data["type"] == "harvest")
		return std::make_unique<HarvestPathRequest>(data, deserializationMemo);
	if(data["type"] == "drink")
		return std::make_unique<DrinkPathRequest>(data, deserializationMemo);
	if(data["type"] == "give plants fluid")
		return std::make_unique<GivePlantsFluidPathRequest>(data, deserializationMemo);
	if(data["type"] == "sleep")
		return std::make_unique<SleepPathRequest>(data, deserializationMemo);
	if(data["type"] == "stockpile")
		return std::make_unique<StockPilePathRequest>(data, deserializationMemo);
	if(data["type"] == "leave area")
		return std::make_unique<LeaveAreaPathRequest>(data, deserializationMemo);
	assert(data["type"] == "uniform");
	return std::make_unique<UniformPathRequest>(data, deserializationMemo);
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
	// Try to reserve blocks that will be occupied during work.
	if (result.useCurrentPosition)
	{
		if (!actors.move_tryToReserveOccupied(actor))
		{
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
	}
	else if (!actors.move_tryToReserveProposedDestination(actor, result.path))
	{
		actors.objective_canNotCompleteSubobjective(actor);
		return;
	}
	if(m_reserve)
	{
		// Try to reserve block which passed predicate.
		if(m_reserveBlockThatPassedPredicate && !actors.canReserve_tryToReserveLocation(actor, result.blockThatPassedPredicate))
		{
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
	}
	if(result.useCurrentPosition)
	{
		assert(result.path.empty());
		actors.objective_execute(actor);
	}
	else
		actors.move_setPath(actor, result.path);
	onSuccess(area, result.blockThatPassedPredicate);
}
void PathRequest::update(PathRequestIndex newIndex)
{
	assert(newIndex.exists());
	m_index = newIndex;
}
ObjectivePathRequest::ObjectivePathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_objective(static_cast<Objective&>(*deserializationMemo.m_objectives[data["objective"]]))
{
	data["reserve"].get_to(m_reserveBlockThatPassedPredicate);
	nlohmann::from_json(data, static_cast<PathRequest&>(*this));
}
Json ObjectivePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["reserve"] = m_reserveBlockThatPassedPredicate;
	return output;
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
NeedPathRequest::NeedPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_objective(static_cast<Objective&>(*deserializationMemo.m_objectives[data["objective"]]))
{
	nlohmann::from_json(data, static_cast<PathRequest&>(*this));
}
Json NeedPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}
