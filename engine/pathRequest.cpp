#include "pathRequest.h"
#include "terrainFacade.hpp"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
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

PathRequest::PathRequest(BlockIndex st, DistanceInBlocks mr, ActorReference ac, ShapeId sh, FactionId ft, MoveTypeId mt, Facing fi, bool d, bool ad, bool rd) :
	start(st), maxRange(mr), actor(ac), shape(sh), faction(ft), moveType(mt), facing(fi), detour(d), adjacent(ad), reserveDestination(rd)
{ }
PathRequest::PathRequest(const Json& data, Area& area)
{
	data["start"].get_to(start);
	data["maxRange"].get_to(maxRange);
	actor.load(data["actor"], area.getActors().m_referenceData);
	data["shape"].get_to(shape);
	data["faction"].get_to(faction);
	data["moveType"].get_to(moveType);
	data["facing"].get_to(facing);
	data["detour"].get_to(detour);
	data["adjacent"].get_to(adjacent);
	data["reserveDestination"].get_to(reserveDestination);
}
PathRequest& PathRequest::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const MoveTypeId& moveType)
{
	std::string type = data["type"];
	if(type == "basic")
		return record(area, moveType, std::make_unique<GoToAnyPathRequest>(data, area));
	if(type == "attack")
		return record(area, moveType, std::make_unique<GetIntoAttackPositionPathRequest>(data, area));
	if(type == "dig")
		return record(area, moveType, std::make_unique<DigPathRequest>(data, area, deserializationMemo));
	if(type == "temperature")
		return record(area, moveType, std::make_unique<GetToSafeTemperaturePathRequest>(data, area, deserializationMemo));
	if(type == "construct")
		return record(area, moveType, std::make_unique<ConstructPathRequest>(data, area, deserializationMemo));
	if(type == "eat")
		return record(area, moveType, std::make_unique<EatPathRequest>(data, area, deserializationMemo));
	if(type == "craft")
		return record(area, moveType, std::make_unique<CraftPathRequest>(data, area, deserializationMemo));
	if(type == "install item")
		return record(area, moveType, std::make_unique<InstallItemPathRequest>(data, area, deserializationMemo));
	if(type == "wander")
		return record(area, moveType, std::make_unique<WanderPathRequest>(data, area, deserializationMemo));
	if(type == "harvest")
		return record(area, moveType, std::make_unique<HarvestPathRequest>(data, area, deserializationMemo));
	if(type == "drink")
		return record(area, moveType, std::make_unique<DrinkPathRequest>(data, area, deserializationMemo));
	if(type == "give plants fluid")
		return record(area, moveType, std::make_unique<GivePlantsFluidPathRequest>(data, area, deserializationMemo));
	if(type == "sleep")
		return record(area, moveType, std::make_unique<SleepPathRequest>(data, area, deserializationMemo));
	if(type == "stockpile")
		return record(area, moveType, std::make_unique<StockPilePathRequest>(data, area, deserializationMemo));
	if(type == "leave area")
		return record(area, moveType, std::make_unique<LeaveAreaPathRequest>(data, area, deserializationMemo));
	assert(type == "uniform");
	return record(area, moveType, std::make_unique<UniformPathRequest>(data, area, deserializationMemo));
}
PathRequest& PathRequest::record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestBreadthFirst> pathRequest)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestNoHuristic(std::move(pathRequest));
	return *pathRequest;
}
PathRequest& PathRequest::record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestDepthFirst> pathRequest)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestWithHuristic(std::move(pathRequest));
	return *pathRequest;
}
void PathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	actors.move_pathRequestCallback(index, result.path, result.useCurrentPosition, reserveDestination);
}
PathRequestDepthFirst::PathRequestDepthFirst(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockIndex hd) :
	PathRequest(start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination),
	huristicDestination(hd)
{ }
PathRequestDepthFirst::PathRequestDepthFirst(const Json& data, Area& area) :
	PathRequest(data, area),
	huristicDestination(data["huristicDestination"].get<BlockIndex>())
{ }
PathRequestBreadthFirst::PathRequestBreadthFirst(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination) :
	PathRequest(start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination)
{ }
PathRequestBreadthFirst::PathRequestBreadthFirst(const Json& data, Area& area) :
	PathRequest(data, area)
{ }
void PathRequestDepthFirst::cancel(Area& area)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).unregisterWithHuristic(*this);
}
void PathRequestDepthFirst::record(Area& area, std::unique_ptr<PathRequestDepthFirst>& pointerToThis)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestWithHuristic(std::move(pointerToThis));
}
void PathRequestBreadthFirst::cancel(Area& area)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).unregisterNoHuristic(*this);
}
void PathRequestBreadthFirst::record(Area& area, std::unique_ptr<PathRequestBreadthFirst>& pointerToThis)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestNoHuristic(std::move(pointerToThis));
}
GoToPathRequest::GoToPathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockIndex d) :
	PathRequestDepthFirst(start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, d),
	destination(d)
{ }
GoToPathRequest::GoToPathRequest(const Json& data, Area& area) :
	PathRequestDepthFirst(data, area),
	destination(data["destination"].get<BlockIndex>())
{ }
GoToAnyPathRequest::GoToAnyPathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockIndex huristicDestination, BlockIndices d) :
	PathRequestDepthFirst(start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, huristicDestination),
	destinations(d)
{ }
GoToAnyPathRequest::GoToAnyPathRequest(const Json& data, Area& area) :
	PathRequestDepthFirst(data, area)
{
	data["destinations"].get_to(destinations);
}
GoToFluidTypePathRequest::GoToFluidTypePathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, FluidTypeId ft) :
	PathRequestBreadthFirst(start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination),
	fluidType(ft)
{ }
GoToFluidTypePathRequest::GoToFluidTypePathRequest(const Json& data, Area& area) :
	PathRequestBreadthFirst(data, area),
	fluidType(data["fluidType"].get<FluidTypeId>())
{ }
GoToBlockDesignationPathRequest::GoToBlockDesignationPathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockDesignation designation) :
	PathRequestBreadthFirst(start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination),
	designation(designation)
{
	assert(faction.exists());
}
GoToEdgePathRequest::GoToEdgePathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination) :
	PathRequestBreadthFirst(start, maxRange, actor, shape, FactionId::null(), moveType, facing, detour, adjacent, reserveDestination)
{ }
GoToEdgePathRequest::GoToEdgePathRequest(const Json& data, Area& area) : PathRequestBreadthFirst(data, area) { }
FindPathResult GoToPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo)
{
		return terrainFacade.findPathTo(memo, start, facing, shape, destination, detour, adjacent, faction);
}
FindPathResult GoToAnyPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo)
{
		return terrainFacade.findPathToAnyOf(memo, start, facing, shape, destinations, huristicDestination, detour, faction);
}
FindPathResult GoToFluidTypePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Blocks& blocks = area.getBlocks();
	auto destinationCondition = [&](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
	{
		return {blocks.fluid_contains(block, fluidType), block};
	};
	constexpr bool anyOccupiedBlock = false;
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, detour, adjacent, faction, maxRange);
}
GoToBlockDesignationPathRequest::GoToBlockDesignationPathRequest(const Json& data, Area& area) :
	PathRequestBreadthFirst(data, area),
	designation(data["designation"].get<BlockDesignation>())
{ }
FindPathResult GoToBlockDesignationPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Blocks& blocks = area.getBlocks();
	auto destinationCondition = [&](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
	{
		return {blocks.designation_has(block, faction, designation), block};
	};
	constexpr bool anyOccupiedBlock = false;
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, detour, adjacent, faction, maxRange);
}
FindPathResult GoToEdgePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Blocks& blocks = area.getBlocks();
	auto destinationCondition = [&](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
	{
		return {blocks.isEdge(block), block};
	};
	constexpr bool anyOccupiedBlock = true;
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, detour, adjacent, faction, maxRange);
}