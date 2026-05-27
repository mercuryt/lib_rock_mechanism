#pragma once

#include "../numericTypes/types.h"
#include "../reference.h"
#include "../designations.h"
#include "../dataStructures/smallSet.h"
#include "../geometry/point3D.h"
#include <utility>
#include "longRange.h"
#include "result.h"

class AreaHasPathsForMoveType;

struct PathRequest
{
	SmallSet<Point3D> path;
	Point3D target;
	Point3D start;
	Point3D huristicDestination;
	Distance maxRange;
	ActorReference actor;
	ShapeId shape;
	FactionId faction;
	MoveTypeId moveType;
	Facing4 facing;
	bool detour = false;
	bool adjacent = false;
	bool anyOccupiedPoint = false;
	bool reserveDestination = false;
	bool returnPath = true;
	PathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool anyOccupiedPoint, bool reserveDestination, Point3D huristicDestination = Point3D::null());
	PathRequest() = default;
	PathRequest(const Json& data, Area& area);
	[[nodiscard]] PathParamaters toParamaters(Area& area) const;
	[[nodiscard]] virtual PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) = 0;
	[[nodiscard]] virtual Json toJson() const;
	// Default writeStep is used for most pathing.
	// TODO: area should be const here.
	virtual void writeStep(Area& area, bool useCurrentLocation);
	virtual void cancel(Area& area);
	virtual ~PathRequest() = default;
	[[nodiscard]] static PathRequest& load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const MoveTypeId moveType);
	[[nodiscard]] static PathRequest& record(Area &area, const MoveTypeId moveType, std::unique_ptr<PathRequest> pathRequest);
};
inline void to_json(Json& data, const PathRequest& pathRequest){ data = pathRequest.toJson(); }
struct GoToPathRequest : public PathRequest
{
	Point3D destination;
	GoToPathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool anyOccupiedPoint, bool reserveDestination, Point3D destination);
	GoToPathRequest(const Json& data, Area& area);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToAnyPathRequest : public PathRequest
{
	CuboidSet destinations;
	GoToAnyPathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool anyOccupiedPoint, bool reserveDestination, Point3D huristicDestination, CuboidSet destinations);
	GoToAnyPathRequest(const Json& data, Area& area);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToFluidTypePathRequest : public PathRequest
{
	FluidTypeId fluidType;
	GoToFluidTypePathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination, FluidTypeId fluidType);
	GoToFluidTypePathRequest(const Json& data, Area& area);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToSpaceDesignationPathRequest : public PathRequest
{
	SpaceDesignation designation;
	GoToSpaceDesignationPathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool anyOccupiedPoint, bool reserveDestination, SpaceDesignation designation);
	GoToSpaceDesignationPathRequest(const Json& data, Area& area);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToEdgePathRequest : public PathRequest
{
	GoToEdgePathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, MoveTypeId moveType, Facing4 facing, bool detour);
	GoToEdgePathRequest(const Json& data, Area& area);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	[[nodiscard]] Json toJson() const override;
};