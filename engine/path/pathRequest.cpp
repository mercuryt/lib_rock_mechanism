#include "pathRequest.h"
#include "terrainFacade.hpp"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../objectives/dig.h"
#include "../objectives/construct.h"
#include "../objectives/getToSafeTemperature.h"
#include "../objectives/eat.h"
#include "../objectives/installItem.h"
#include "../objectives/wander.h"
#include "../objectives/harvest.h"
#include "../objectives/drink.h"
#include "../objectives/givePlantsFluid.h"
#include "../objectives/sleep.h"
#include "../objectives/stockpile.h"
#include "../objectives/leaveArea.h"
#include "../objectives/uniform.h"

PathRequest::PathRequest(Point3D st, Distance mr, ActorReference ac, ShapeId sh, FactionId ft, MoveTypeId mt, Facing4 fi, bool d, bool ad, bool rd) :
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
Json PathRequest::toJson() const
{
	return{
		{"start", start},
		{"maxRange", maxRange},
		{"actor", actor},
		{"shape", shape},
		{"faction", faction},
		{"moveType", moveType},
		{"facing", facing},
		{"detour", detour},
		{"adjacent", adjacent},
		{"reserveDestination", reserveDestination},
	};
}
PathRequest& PathRequest::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const MoveTypeId& moveType)
{
	std::string type = data["type"].get<std::string>();
	if(type == "goTo")
		return record(area, moveType, std::make_unique<GoToPathRequest>(data, area));
	if(type == "goToAny")
		return record(area, moveType, std::make_unique<GoToAnyPathRequest>(data, area));
	if(type == "goToFluidType")
		return record(area, moveType, std::make_unique<GoToFluidTypePathRequest>(data, area));
	if(type == "goToSpaceDesignation")
		return record(area, moveType, std::make_unique<GoToSpaceDesignationPathRequest>(data, area));
	if(type == "goToEdge")
		return record(area, moveType, std::make_unique<GoToEdgePathRequest>(data, area));
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
	PathRequest& output = *pathRequest;
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestNoHuristic(std::move(pathRequest));
	return output;
}
PathRequest& PathRequest::record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestDepthFirst> pathRequest)
{
	PathRequest& output = *pathRequest;
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestWithHuristic(std::move(pathRequest));
	return output;
}
void PathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	actors.move_pathRequestCallback(index, result.path, result.useCurrentPosition, reserveDestination);
}
PathRequestBreadthFirst::PathRequestBreadthFirst(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination) :
	PathRequest(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _reserveDestination)
{ }
PathRequestBreadthFirst::PathRequestBreadthFirst(const Json& data, Area& area) :
	PathRequest(data, area)
{ }
void PathRequestBreadthFirst::cancel(Area& area)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).unregisterNoHuristic(*this);
}
void PathRequestBreadthFirst::record(Area& area, std::unique_ptr<PathRequestBreadthFirst>& pointerToThis)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestNoHuristic(std::move(pointerToThis));
}
// This method does nothing but is included for interface symetry with depth first.
Json PathRequestBreadthFirst::toJson() const
{
	return PathRequest::toJson();
}
PathRequestDepthFirst::PathRequestDepthFirst(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination, Point3D hd) :
	PathRequest(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _reserveDestination), huristicDestination(hd)
{ }
PathRequestDepthFirst::PathRequestDepthFirst(const Json& data, Area& area) :
	PathRequest(data, area),
	huristicDestination(data["huristicDestination"].get<Point3D>())
{ }
void PathRequestDepthFirst::cancel(Area& area)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).unregisterWithHuristic(*this);
}
void PathRequestDepthFirst::record(Area& area, std::unique_ptr<PathRequestDepthFirst>& pointerToThis)
{
	area.m_hasTerrainFacades.getForMoveType(moveType).registerPathRequestWithHuristic(std::move(pointerToThis));
}
Json PathRequestDepthFirst::toJson() const
{
	Json output = PathRequest::toJson();
	output["huristicDestination"] = huristicDestination;
	return output;
}
GoToPathRequest::GoToPathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination, Point3D d) :
	PathRequestDepthFirst(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _reserveDestination, d),
	destination(d)
{ }
GoToPathRequest::GoToPathRequest(const Json& data, Area& area) :
	PathRequestDepthFirst(data, area),
	destination(data["destination"].get<Point3D>())
{ }
FindPathResult GoToPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo)
{
		return terrainFacade.findPathTo(memo, start, facing, shape, destination, detour, adjacent, faction);
}
Json GoToPathRequest::toJson() const
{
	Json output = PathRequestDepthFirst::toJson();
	output["destination"] = destination;
	output["type"] = "goTo";
	return output;
}
GoToAnyPathRequest::GoToAnyPathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination, Point3D _huristicDestination, CuboidSet d) :
	PathRequestDepthFirst(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _reserveDestination, _huristicDestination),
	destinations(d)
{ }
GoToAnyPathRequest::GoToAnyPathRequest(const Json& data, Area& area) :
	PathRequestDepthFirst(data, area)
{
	data["destinations"].get_to(destinations);
}
FindPathResult GoToAnyPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo)
{
	if(adjacent)
		return terrainFacade.findPathToAnyOf<false, true>(memo, start, facing, shape, destinations, huristicDestination, detour, faction);
	else
		return terrainFacade.findPathToAnyOf<false, false>(memo, start, facing, shape, destinations, huristicDestination, detour, faction);
}
Json GoToAnyPathRequest::toJson() const
{
	Json output = PathRequestDepthFirst::toJson();
	output["destinations"] = destinations;
	output["type"] = "goToAny";
	return output;
}
GoToFluidTypePathRequest::GoToFluidTypePathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination, FluidTypeId ft) :
	PathRequestBreadthFirst(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _reserveDestination),
	fluidType(ft)
{ }
GoToFluidTypePathRequest::GoToFluidTypePathRequest(const Json& data, Area& area) :
	PathRequestBreadthFirst(data, area),
	fluidType(data["fluidType"].get<FluidTypeId>())
{ }
FindPathResult GoToFluidTypePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Space& space = area.getSpace();
	auto destinationCondition = [&](const Cuboid& cuboid) -> std::pair<bool, Point3D>
	{
		const Point3D point = space.fluid_containsPoint(cuboid, fluidType);
		return {point.exists(), point};
	};
	constexpr bool anyOccupiedPoint = false;
	constexpr bool useAdjacent = true;
	return terrainFacade.findPathToConditionBreadthFirst<decltype(destinationCondition), anyOccupiedPoint, useAdjacent>(destinationCondition, memo, start, facing, shape, detour, faction, maxRange);
}
Json GoToFluidTypePathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["fluidType"] = fluidType;
	output["type"] = "goToFluidType";
	return output;
}
GoToSpaceDesignationPathRequest::GoToSpaceDesignationPathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination, SpaceDesignation _designation) :
	PathRequestBreadthFirst(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _reserveDestination),
	designation(_designation)
{
	assert(faction.exists());
}
FindPathResult GoToSpaceDesignationPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Space& space = area.getSpace();
	constexpr bool anyOccupiedPoint = false;
	if(adjacent)
	{
		auto destinationCondition = [&](const Cuboid& cuboid) -> std::pair<bool, Point3D>
		{
			Point3D point = space.designation_hasPoint(cuboid, faction, designation);
			return {point.exists(), point};
		};
		return terrainFacade.findPathToConditionBreadthFirst<decltype(destinationCondition), anyOccupiedPoint, true>(destinationCondition, memo, start, facing, shape, detour, faction, maxRange);
	}
	else
	{
		auto destinationCondition = [&](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D>
		{
			return {space.designation_has(point, faction, designation), point};
		};
		return terrainFacade.findPathToConditionBreadthFirst<decltype(destinationCondition), anyOccupiedPoint, false>(destinationCondition, memo, start, facing, shape, detour, faction, maxRange);
	}
}
Json GoToSpaceDesignationPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["designation"] = designation;
	output["type"] = "goToSpaceDesignation";
	return output;
}
GoToEdgePathRequest::GoToEdgePathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination) :
	PathRequestBreadthFirst(_start, _maxRange, _actor, _shape, FactionId::null(), _moveType, _facing, _detour, _adjacent, _reserveDestination)
{ }
GoToEdgePathRequest::GoToEdgePathRequest(const Json& data, Area& area) : PathRequestBreadthFirst(data, area) { }
GoToSpaceDesignationPathRequest::GoToSpaceDesignationPathRequest(const Json& data, Area& area) :
	PathRequestBreadthFirst(data, area),
	designation(data["designation"].get<SpaceDesignation>())
{ }
FindPathResult GoToEdgePathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Space& space = area.getSpace();
	auto destinationCondition = [&](const Cuboid& cuboid) -> std::pair<bool, Point3D>
	{
		return {space.isEdge(cuboid), cuboid.m_high};
	};
	constexpr bool anyOccupiedPoint = true;
	constexpr bool anyAdjacent = false;
	return terrainFacade.findPathToConditionBreadthFirst<decltype(destinationCondition), anyOccupiedPoint, anyAdjacent>(destinationCondition, memo, start, facing, shape, detour, faction, maxRange);
}
Json GoToEdgePathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["type"] = "goToEdge";
	return output;
}