#include "pathRequest.h"
#include "longRange.hpp"
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

PathRequest::PathRequest(Point3D st, Distance mr, ActorReference ac, ShapeId sh, FactionId ft, MoveTypeId mt, Facing4 fi, bool d, bool ad, bool aop, bool rd, Point3D hd) :
	start(st), huristicDestination(hd), maxRange(mr), actor(ac), shape(sh), faction(ft), moveType(mt), facing(fi), detour(d), adjacent(ad), anyOccupiedPoint(aop), reserveDestination(rd)
{
	// Either adjacent or any occupied point. adjacent also include occupied.
	assert((int)adjacent + (int)anyOccupiedPoint < 2);
}
PathRequest::PathRequest(const Json& data, Area& area)
{
	data["start"].get_to(start);
	data["huristicDestination"].get_to(huristicDestination);
	data["maxRange"].get_to(maxRange);
	actor.load(data["actor"], area.getActors().m_referenceData);
	data["shape"].get_to(shape);
	data["faction"].get_to(faction);
	data["moveType"].get_to(moveType);
	data["facing"].get_to(facing);
	data["detour"].get_to(detour);
	data["adjacent"].get_to(adjacent);
	data["anyOccupiedPoint"].get_to(anyOccupiedPoint);
	data["reserveDestination"].get_to(reserveDestination);
	data["returnPath"].get_to(returnPath);
}
PathParamaters PathRequest::toParamaters(Area& area) const
{
	PathParamaters output({
		.area = area,
		.start = start,
		.huristicDestination = huristicDestination,
		.shape = shape,
		.faction = faction,
		.maxRange = maxRange,
		.moveType = moveType,
		.startFacing = facing,
		.depthFirst = huristicDestination.exists(),
		.detour = detour,
		.adjacent = adjacent,
		.anyOccupiedPoint = anyOccupiedPoint,
		.returnPath = returnPath
	});
	if(detour)
	{
		const Actors& actors = area.getActors();
		output.occupied = actors.getOccupied(actor.getIndex(actors.m_referenceData));
	}
	return output;
}
Json PathRequest::toJson() const
{
	return{
		{"start", start},
		{"huristicDestination", huristicDestination},
		{"maxRange", maxRange},
		{"actor", actor},
		{"shape", shape},
		{"faction", faction},
		{"moveType", moveType},
		{"facing", facing},
		{"detour", detour},
		{"adjacent", adjacent},
		{"anyOccupiedPoint", anyOccupiedPoint},
		{"reserveDestination", reserveDestination},
		{"returnPath", returnPath},
	};
}
PathRequest& PathRequest::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const MoveTypeId moveType)
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
PathRequest& PathRequest::record(Area &area, const MoveTypeId moveType, std::unique_ptr<PathRequest> pathRequest)
{
	PathRequest& output = *pathRequest;
	area.m_hasPaths.get(area, moveType).recordPathRequest(std::move(pathRequest));
	return output;
}
void PathRequest::writeStep(Area& area, bool useCurrentPosition)
{
	Actors& actors = area.getActors();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	actors.move_pathRequestCallback(index, path, useCurrentPosition, reserveDestination);
}
void PathRequest::cancel(Area& area)
{
	area.m_hasPaths.get(area, moveType).cancelPathRequest(*this);
}
GoToPathRequest::GoToPathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _anyOccupiedPoint, bool _reserveDestination, Point3D d) :
	PathRequest(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _anyOccupiedPoint, _reserveDestination, d),
	destination(d)
{ }
GoToPathRequest::GoToPathRequest(const Json& data, Area& area) :
	PathRequest(data, area),
	destination(data["destination"].get<Point3D>())
{ }
PathResult GoToPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	return hasPaths.pathTo(toParamaters(area));
}
Json GoToPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["destination"] = destination;
	output["type"] = "goTo";
	return output;
}
GoToAnyPathRequest::GoToAnyPathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _useAnyOccupied, bool _reserveDestination, Point3D _huristicDestination, CuboidSet d) :
	PathRequest(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _useAnyOccupied, _reserveDestination, _huristicDestination),
	destinations(d)
{ }
GoToAnyPathRequest::GoToAnyPathRequest(const Json& data, Area& area) :
	PathRequest(data, area)
{
	data["destinations"].get_to(destinations);
}
PathResult GoToAnyPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	auto longRangeCondition = [&](const Cuboid cuboid) -> bool { return destinations.intersects(cuboid); };
	if(adjacent || anyOccupiedPoint)
	{
		auto shortRangeCondition = [&](const Cuboid cuboid) -> Point3D
		{
			for(const Cuboid destinationCuboid : destinations)
				if(destinationCuboid.intersects(cuboid))
					return destinationCuboid.intersectionPoint(cuboid);
			return Point3D::null();
		};
		if(adjacent)
			return longRangePath::getPathAdjacent(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
		else
			return longRangePath::getPathAnyOccupied(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
	}
	else
	{
		auto shortRangeCondition = [&](const Point3D point, const Facing4) -> Point3D { return destinations.contains(point) ? point : Point3D::null(); };
		return longRangePath::getPath(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
	}
}
Json GoToAnyPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["destinations"] = destinations;
	output["type"] = "goToAny";
	return output;
}
GoToFluidTypePathRequest::GoToFluidTypePathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _reserveDestination, FluidTypeId ft) :
	PathRequest(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, false, _reserveDestination),
	fluidType(ft)
{ }
GoToFluidTypePathRequest::GoToFluidTypePathRequest(const Json& data, Area& area) :
	PathRequest(data, area),
	fluidType(data["fluidType"].get<FluidTypeId>())
{ }
PathResult GoToFluidTypePathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Space& space = area.getSpace();
	auto longRangeCondition = [&](const Cuboid cuboid) -> bool { return space.fluid_containsPoint(cuboid, fluidType) != Point3D::null(); };
	if(adjacent || anyOccupiedPoint)
	{
		auto shortRangeCondition = [&](const Cuboid cuboid) -> Point3D { return space.fluid_containsPoint(cuboid, fluidType); };
		if(adjacent)
			return longRangePath::getPathAdjacent(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
		else
			return longRangePath::getPathAnyOccupied(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
	}
	else
	{
		auto shortRangeCondition = [&](const Point3D point, const Facing4) -> Point3D { return space.fluid_containsPoint(point, fluidType); };
		return longRangePath::getPath(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
	}
}
Json GoToFluidTypePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["fluidType"] = fluidType;
	output["type"] = "goToFluidType";
	return output;
}
GoToSpaceDesignationPathRequest::GoToSpaceDesignationPathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, FactionId _faction, MoveTypeId _moveType, Facing4 _facing, bool _detour, bool _adjacent, bool _anyOccupiedPoint, bool _reserveDestination, SpaceDesignation _designation) :
	PathRequest(_start, _maxRange, _actor, _shape, _faction, _moveType, _facing, _detour, _adjacent, _anyOccupiedPoint, _reserveDestination),
	designation(_designation)
{
	assert(faction.exists());
}
PathResult GoToSpaceDesignationPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	assert(!huristicDestination.exists());
	Space& space = area.getSpace();
	auto longRangeCondition = [&](const Cuboid cuboid) -> bool { return space.designation_hasPoint(cuboid, faction, designation) != Point3D::null(); };
	if(adjacent || anyOccupiedPoint)
	{
		auto shortRangeCondition = [&](const Cuboid cuboid) -> Point3D { return space.designation_hasPoint(cuboid, faction, designation); };
		if(adjacent)
			return longRangePath::getPathAdjacent(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
		else
			return longRangePath::getPathAnyOccupied(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
	}
	else
	{
		auto shortRangeCondition = [&](const Point3D point, const Facing4) -> Point3D { return space.designation_has(point, faction, designation) ? point : Point3D::null(); };
		return longRangePath::getPath(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
	}
}
Json GoToSpaceDesignationPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["designation"] = designation;
	output["type"] = "goToSpaceDesignation";
	return output;
}
GoToEdgePathRequest::GoToEdgePathRequest(Point3D _start, Distance _maxRange, ActorReference _actor, ShapeId _shape, MoveTypeId _moveType, Facing4 _facing, bool _detour) :
	PathRequest(_start, _maxRange, _actor, _shape, FactionId::null(), _moveType, _facing, _detour, false, false, false)
{ }
GoToEdgePathRequest::GoToEdgePathRequest(const Json& data, Area& area) : PathRequest(data, area) { }
GoToSpaceDesignationPathRequest::GoToSpaceDesignationPathRequest(const Json& data, Area& area) :
	PathRequest(data, area),
	designation(data["designation"].get<SpaceDesignation>())
{ }
PathResult GoToEdgePathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Space& space = area.getSpace();
	Cuboid boundry = space.boundry();
	auto longRangeCondition = [&](const Cuboid cuboid) -> bool { return boundry.isTouchingFaceFromInside(cuboid); };
	auto shortRangeCondition = [&](const Point3D point, const Facing4 shortRangeFacing) -> Point3D
	{
		Cuboid shapeBoundry = Shape::getBoundryAtWithFacing(shape, space, point, shortRangeFacing);
		return boundry.isTouchingFaceFromInside(shapeBoundry) ? point : Point3D::null();
	};
	assert(!adjacent);
	assert(!anyOccupiedPoint);
	assert(faction.empty());
	assert(huristicDestination.empty());
	return longRangePath::getPath(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
}
Json GoToEdgePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["type"] = "goToEdge";
	return output;
}