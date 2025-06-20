#include "pathRequest.h"
#include "area/area.h"
#include "deserializationMemo.h"
#include "numericTypes/types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "items/items.h"
#include "plants.h"
#include "findPathInnerLoops.h"
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
void PathRequest::create(Area& area, const ActorIndex& actor, const BlockIndex& start, const Facing4& startFacing, DestinationCondition destination, bool detour, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve)
{
	// Store paramaters for serialization.
	m_reserve = reserve;
	if(m_reserve)
		assert(m_unreserved);
	assert(actor.exists());
	assert(m_actor.empty());
	m_actor = actor;
	m_detour = detour;
	m_maxRange = maxRange;
	m_huristicDestination = huristicDestination;
	Actors& actors = area.getActors();
	MoveTypeId moveType = actors.isLeading(actor) ? actors.lineLead_getMoveType(actor) : actors.getMoveType(actor);
	TerrainFacade& terrainFacade = area.m_hasTerrainFacades.getForMoveType(moveType);
	if(huristicDestination.empty())
		terrainFacade.registerPathRequestNoHuristic(*this, m_actor, start, startFacing, access, destination, *this);
	else
		terrainFacade.registerPathRequestWithHuristic(start, startFacing, access, destination, huristicDestination, *this);
	assert(m_index.exists());
}
void PathRequest::createGoTo(Area& area, const ActorIndex& actor, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	Actors& actors = area.getActors();
	createGoToFrom(area, actor, actors.getLocation(actor), actors.getFacing(actor), destination, detour, unreserved, maxRange, reserve);
}
void PathRequest::createGoToFrom(Area& area, const ActorIndex& actor, const BlockIndex& start, const Facing4& startFacing, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	m_unreserved = unreserved;
	m_destination = destination;
	Blocks& blocks = area.getBlocks();
	FactionId faction;
	if(unreserved)
		faction = area.getActors().getFaction(actor);
	// TODO: Pathing to a specific block that is reserved should do nothing but instead we iterate every block in range.
	// 	fix by adding a predicate to path request which prevents the call to findPath.
	DestinationCondition destinationCondition = [destination, &blocks, faction](const BlockIndex& index, const Facing4&) {
		bool result = index == destination && (faction.empty() || !blocks.isReserved(index, faction));
		return std::make_pair(result, index);
	};
	create(area, actor, start, startFacing, destinationCondition, detour, maxRange, destination, reserve);
}
void PathRequest::createGoToAnyOf(Area& area, const ActorIndex& actor, BlockIndices destinations, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve)
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
		FactionId faction = area.getActors().getFaction(actor);
		destinations.remove_if([&blocks, faction](const BlockIndex& index){ return !blocks.isReserved(index, faction); });
	}
	MoveTypeId moveType = actors.getMoveType(actor);
	destinations.remove_if([&blocks, &moveType](const BlockIndex& index){
		return !blocks.shape_anythingCanEnterEver(index) || !blocks.shape_moveTypeCanEnter(index, moveType);
	});
	//TODO: handle no destinations.
	assert(!destinations.empty());
	if(Shape::getIsMultiTile(shape))
		destinationCondition = [shape, &area, &actors, destinations, actor, unreserved](const BlockIndex& location, const Facing4& facing)
		{
			// Multi tile actors must be checked if any of the spots to be occupied are reserved.
			// TODO: since we already have shape and faction calling into actors is probably not the best choice here.
			if(!unreserved || actors.canReserve_canReserveLocation(actor, location, facing))
			{
				BlockIndices blocks = Shape::getBlocksOccupiedAt(shape, area.getBlocks(), location, facing);
				for(BlockIndex block : blocks)
					if(destinations.contains(block))
						return std::make_pair(true, location);
			}
			return std::make_pair(false, BlockIndex::null());
		};
	else
		destinationCondition = [destinations](const BlockIndex& index, const Facing4&) {
				if(destinations.contains(index))
					return std::make_pair(true, index);
				return std::make_pair(false, BlockIndex::null());
			};
	create(area, actor, actors.getLocation(actor), actors.getFacing(actor), destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToLocation(Area& area, const ActorIndex& actor, const BlockIndex& destination, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	m_adjacent = true;
	m_destination = destination;
	BlockIndices candidates;
	Blocks& blocks = area.getBlocks();
	for(BlockIndex block : blocks.getAdjacentWithEdgeAndCornerAdjacent(destination))
		candidates.add(block);
	createGoToAnyOf(area, actor, candidates, detour, unreserved, maxRange, destination, reserve);

}
void PathRequest::createGoAdjacentToActor(Area& area, const ActorIndex& actor, const ActorIndex& other, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	Actors& actors = area.getActors();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto otherAdjacent = actors.getAdjacentBlocks(other);
	createGoToAnyOf(area, actor, otherAdjacent, detour, unreserved, maxRange, actors.getLocation(other), reserve);
}
void PathRequest::createGoAdjacentToItem(Area& area, const ActorIndex& actor, const ItemIndex& item, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	Items& items = area.getItems();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto itemAdjacent = items.getAdjacentBlocks(item);
	createGoToAnyOf(area, actor, itemAdjacent, detour, unreserved, maxRange, items.getLocation(item), reserve);
}
void PathRequest::createGoAdjacentToPlant(Area& area, const ActorIndex& actor, const PlantIndex& plant, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	Plants& plants = area.getPlants();
	//TODO: make getAdjacentBlocks return a vector rather then a set.
	auto plantAdjacent = plants.getAdjacentBlocks(plant);
	createGoToAnyOf(area, actor, plantAdjacent, detour, unreserved, maxRange, plants.getLocation(plant), reserve);
}
void PathRequest::createGoAdjacentToPolymorphic(Area& area, const ActorIndex& actor, const ActorOrItemIndex& actorOrItem, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	if(actorOrItem.isActor())
		createGoAdjacentToActor(area, actor, actorOrItem.getActor(), detour, unreserved, maxRange, reserve);
	else
		createGoAdjacentToItem(area, actor, actorOrItem.getItem(), detour, unreserved, maxRange, reserve);
}
void PathRequest::createGoAdjacentToFluidType(Area& area, const ActorIndex& actor, const FluidTypeId& fluidType, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	m_fluidType = fluidType;
	Blocks& blocks = area.getBlocks();
	std::function<bool(BlockIndex)> condition = [&blocks, fluidType](const BlockIndex& index){
		return blocks.fluid_contains(index, fluidType);
	};
	createGoAdjacentToCondition(area, actor, condition, detour, unreserved, maxRange, BlockIndex::null(), reserve);
}
void PathRequest::createGoAdjacentToDesignation(Area& area, const ActorIndex& actor, const BlockDesignation& designation, bool detour, bool unreserved, const DistanceInBlocks& maxRange, bool reserve)
{
	m_designation = designation;
	FactionId faction = area.getActors().getFaction(actor);
	AreaHasBlockDesignationsForFaction& designations = area.m_blockDesignations.getForFaction(faction);
	auto offset = designations.getOffsetForDesignation(designation);
	std::function<bool(BlockIndex)> predicate = [&, offset](const BlockIndex& block){
		return designations.checkWithOffset(offset, block);
	};
	createGoAdjacentToCondition(area, actor, predicate, detour, unreserved, maxRange, BlockIndex::null(), reserve);
}
void PathRequest::createGoToEdge(Area& area, const ActorIndex& actor, bool detour)
{
	Blocks& blocks = area.getBlocks();
	DestinationCondition condition = [&blocks](const BlockIndex& index, const Facing4&)->std::pair<bool, BlockIndex>{ return {blocks.isEdge(index), index}; };
	createGoToCondition(area, actor, condition, detour, false, DistanceInBlocks::max(), BlockIndex::null());
}
void PathRequest::createGoToCondition(Area& area, const ActorIndex& actor, DestinationCondition condition, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve)
{
	FactionId faction;
	if(unreserved)
		faction = area.getActors().getFaction(actor);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	DestinationCondition destinationCondition;
	const ShapeId& shape = actors.getShape(actor);
	//TODO: use seperate lambdas rather then always passing unreserved and faction.
	if(Shape::getIsMultiTile(shape))
	{
		destinationCondition = [&blocks, condition, unreserved, faction, shape](const BlockIndex &index, const Facing4 &facing) mutable -> std::pair<bool, BlockIndex>
		{
			std::pair<bool, BlockIndex> output = {false, BlockIndex::null()};
			for(const BlockIndex& block : Shape::getBlocksOccupiedAt(shape, blocks, index, facing))
			{
				if(unreserved && blocks.isReserved(index, faction))
					return {false, BlockIndex::null()};
				// Store rather then return imideatly because we have to make sure none of the blocks are reserved.
				if(!output.first && blocks.isEdge(block))
					output = {true, block};
			}
			return output;
		};
	}
	else
	{
		destinationCondition = [&blocks, condition, unreserved, faction](const BlockIndex &index, const Facing4 &facing) mutable
		{
			bool result = condition(index, facing).first && (!unreserved || !blocks.isReserved(index, faction));
			return std::make_pair(result, index);
		};
	}
	create(area, actor, actors.getLocation(actor), actors.getFacing(actor), destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToConditionFrom(Area& area, const ActorIndex& actor, const BlockIndex& start, const Facing4& startFacing, std::function<bool(const BlockIndex&)> condition, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve)
{
	FactionId faction;
	if(unreserved)
	{
		faction = area.getActors().getFaction(actor);
		assert(faction.exists());
		m_unreserved = true;
	}
	Actors& actors = area.getActors();
	ShapeId shape = actors.getShape(actor);
	DestinationCondition destinationCondition;
	Blocks& blocks = area.getBlocks();
	BlockIndices closed;
	//TODO: use seperate lambdas rather then always passing unreserved and faction?
	if(Shape::getIsMultiTile(shape))
	{
		destinationCondition = [shape, &blocks, &actors, closed, condition, unreserved, faction, actor](const BlockIndex& location, const Facing4& facing) mutable -> std::pair<bool, BlockIndex>
		{
			// TODO: since we already have shape and faction calling into actors is probably not the best choice here.
			if(!unreserved || actors.canReserve_canReserveLocation(actor, location, facing))
				for(BlockIndex block : Shape::getBlocksWhichWouldBeAdjacentAt(shape, blocks, location, facing))
				{
					if(closed.contains(block))
						continue;
					closed.add(block);
					if(condition(block))
						return std::make_pair(true, block);
				}
			return std::make_pair(false, BlockIndex::null());
		};
	}
	else
	{
		destinationCondition = [&blocks, closed, condition, unreserved, faction](const BlockIndex& index, const Facing4&) mutable -> std::pair<bool, BlockIndex>
		{
			if(!unreserved || !blocks.isReserved(index, faction))
				for(BlockIndex block : blocks.getAdjacentWithEdgeAndCornerAdjacent(index))
				{
					if(closed.contains(block))
						continue;
					closed.add(block);
					if(condition(block))
						return std::make_pair(true, block);
				}
			return std::make_pair(false, BlockIndex::null());
		};
	}
	create(area, actor, start, startFacing, destinationCondition, detour, maxRange, huristicDestination, reserve);
}
void PathRequest::createGoAdjacentToCondition(Area& area, const ActorIndex& actor, std::function<bool(const BlockIndex&)> condition, bool detour, bool unreserved, const DistanceInBlocks& maxRange, const BlockIndex& huristicDestination, bool reserve)
{
	Actors& actors = area.getActors();
	createGoAdjacentToConditionFrom(area, actor, actors.getLocation(actor), actors.getFacing(actor), condition, detour, unreserved, maxRange, huristicDestination, reserve);
}
void PathRequest::cancel(Area& area, const ActorIndex& actor)
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
	m_maxRange = DistanceInBlocks::null();
	m_designation = BlockDesignation::BLOCK_DESIGNATION_MAX;
	m_detour = false;
	m_unreserved = false;
	m_adjacent = false;
	m_reserve = false;
	m_reserveBlockThatPassesPredicate = false;
}
void PathRequest::callback(Area& area, const FindPathResult& result)
{
	area.getActors().move_pathRequestCallback(m_actor, result.path, result.useCurrentPosition, m_reserve);
}
FindPathResult PathRequest::findPathDepthFirst(const Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo, const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& start, const Facing4& facing, const BlockIndex& huristicDestination)
{
	auto destinationCondition = [huristicDestination](const BlockIndex& block, const Facing4& facing) -> std::pair<bool, BlockIndex> { return {block == huristicDestination, block}; };
	memo.setDestination(huristicDestination);
	FactionId faction;
	if(m_unreserved)
		faction = area.getActors().getFaction(m_actor);
	constexpr bool anyOccupiedBlock = false;
	constexpr DistanceInBlocks maxRange = DistanceInBlocks::max();
	return PathInnerLoops::findPath<anyOccupiedBlock>(destinationCondition, area, terrainFacade, memo, shape, moveType, start, facing, m_detour, m_adjacent, faction, maxRange);
}
PathRequest::PathRequest(const Json& data)
{
	data["index"].get_to(m_index);
	data["actor"].get_to(m_actor);
	if(data.contains("fluidType"))
		data["fluidType"].get_to(m_fluidType);
	if(data.contains("destination"))
		data["destination"].get_to(m_destination);
	if(data.contains("huristicDestination"))
		data["huristicDestination"].get_to(m_huristicDestination);
	if(data.contains("maxRange"))
		data["maxRange"].get_to(m_maxRange);
	if(data.contains("designation"))
		data["designation"].get_to(m_designation);
	if(data.contains("detour"))
		data["detour"].get_to(m_detour);
	if(data.contains("unreserved"))
		data["unreserved"].get_to(m_unreserved);
	if(data.contains("adjacent"))
		data["adjacent"].get_to(m_adjacent);
	if(data.contains("reserve"))
		data["reserve"].get_to(m_reserve);
}
Json PathRequest::toJson() const
{
	Json output {{"index", m_index}, {"actor", m_actor}, {"type", "basic"}};
	if(m_fluidType.exists())
		output["fluidType"] = m_fluidType;
	if(m_destination.exists())
		output["destination"] = m_destination;
	if(m_huristicDestination.exists())
		output["huristicDestination"] = m_huristicDestination;
	if(m_maxRange.exists())
		output["maxRange"] = m_maxRange;
	if(m_designation != BlockDesignation::BLOCK_DESIGNATION_MAX)
		output["designation"] = m_designation;
	if(m_detour)
		output["detour"] = m_detour;
	if(m_unreserved)
		output["unreserved"] = m_unreserved;
	if(m_adjacent)
		output["adjacent"] = m_adjacent;
	if(m_reserve)
		output["reserve"] = m_reserve;
	return output;
}
std::unique_ptr<PathRequest> PathRequest::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	std::wstring type = data["type"];
	if(type == "basic")
		return std::make_unique<PathRequest>(data);
	if(type == "attack")
		return std::make_unique<GetIntoAttackPositionPathRequest>(data, area);
	if(type == "dig")
		return std::make_unique<DigPathRequest>(data, deserializationMemo);
	if(type == "temperature")
		return std::make_unique<GetToSafeTemperaturePathRequest>(data, deserializationMemo);
	if(type == "construct")
		return std::make_unique<ConstructPathRequest>(data, deserializationMemo);
	if(type == "eat")
		return std::make_unique<EatPathRequest>(data, deserializationMemo);
	if(type == "craft")
		return std::make_unique<CraftPathRequest>(data, deserializationMemo);
	if(type == "install item")
		return std::make_unique<InstallItemPathRequest>(data, deserializationMemo);
	if(type == "wander")
		return std::make_unique<WanderPathRequest>(data, deserializationMemo);
	if(type == "harvest")
		return std::make_unique<HarvestPathRequest>(data, deserializationMemo);
	if(type == "drink")
		return std::make_unique<DrinkPathRequest>(data, deserializationMemo);
	if(type == "give plants fluid")
		return std::make_unique<GivePlantsFluidPathRequest>(data, deserializationMemo);
	if(type == "sleep")
		return std::make_unique<SleepPathRequest>(data, deserializationMemo);
	if(type == "stockpile")
		return std::make_unique<StockPilePathRequest>(data, deserializationMemo);
	if(type == "leave area")
		return std::make_unique<LeaveAreaPathRequest>(data, deserializationMemo);
	assert(type == "uniform");
	return std::make_unique<UniformPathRequest>(data, deserializationMemo);
}
void ObjectivePathRequest::callback(Area& area, const FindPathResult& result)
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
void PathRequest::update(const PathRequestIndex& newIndex)
{
	assert(newIndex.exists());
	m_index = newIndex;
}
ObjectivePathRequest::ObjectivePathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_objective(static_cast<Objective&>(*deserializationMemo.m_objectives[data["objective"]]))
{
	data["reserve"].get_to(m_reserveBlockThatPassedPredicate);
	//nlohmann::from_json(data, static_cast<PathRequest&>(*this));
}
Json ObjectivePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["reserve"] = m_reserveBlockThatPassedPredicate;
	return output;
}
void NeedPathRequest::callback(Area& area, const FindPathResult& result)
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
	PathRequest(data),
	m_objective(static_cast<Objective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json NeedPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}
