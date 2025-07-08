#pragma once

#include "numericTypes/types.h"
#include "reference.h"
#include "designations.h"

class TerrainFacade;
class FindPathResult;
class PathMemoDepthFirst;
class PathMemoBreadthFirst;
class PathRequestBreadthFirst;
class PathRequestDepthFirst;

struct PathRequest
{
	Point3D start;
	Distance maxRange;
	ActorReference actor;
	ShapeId shape;
	FactionId faction;
	MoveTypeId moveType;
	Facing4 facing;
	bool detour = false;
	bool adjacent = false;
	bool reserveDestination = false;
	PathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination);
	PathRequest() = default;
	PathRequest(const Json& data, Area& area);
	[[nodiscard]] virtual Json toJson() const;
	[[nodiscard]] static PathRequest& load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const MoveTypeId& moveType);
	[[nodiscard]] static PathRequest& record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestBreadthFirst> pathRequest);
	[[nodiscard]] static PathRequest& record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestDepthFirst> pathRequest);
	virtual void writeStep(Area& area, FindPathResult& result);
	virtual void cancel(Area& area) = 0;
	virtual ~PathRequest() = default;
};
inline void to_json(Json& data, const PathRequest& pathRequest){ data = pathRequest.toJson(); }
struct PathRequestDepthFirst : public PathRequest
{
	Point3D huristicDestination;
	PathRequestDepthFirst(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination, Point3D huristicDestination);
	PathRequestDepthFirst() = default;
	PathRequestDepthFirst(const Json& data, Area& area);
	void cancel(Area& area) override;
	void record(Area& area, std::unique_ptr<PathRequestDepthFirst>& pointerToThis);
	[[nodiscard]] virtual FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) = 0;
	[[nodiscard]] virtual Json toJson() const override;
};
struct PathRequestBreadthFirst : public PathRequest
{
	PathRequestBreadthFirst(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination);
	PathRequestBreadthFirst() = default;
	PathRequestBreadthFirst(const Json& data, Area& area);
	void cancel(Area& area) override;
	void record(Area& area, std::unique_ptr<PathRequestBreadthFirst>& pointerToThis);
	[[nodiscard]] virtual FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) = 0;
	[[nodiscard]] virtual Json toJson() const override;
};
struct GoToPathRequest : public PathRequestDepthFirst
{
	Point3D destination;
	GoToPathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination, Point3D destination);
	GoToPathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToAnyPathRequest : public PathRequestDepthFirst
{
	SmallSet<Point3D> destinations;
	GoToAnyPathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination, Point3D huristicDestination, SmallSet<Point3D> destinations);
	GoToAnyPathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToFluidTypePathRequest : public PathRequestBreadthFirst
{
	FluidTypeId fluidType;
	GoToFluidTypePathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination, FluidTypeId fluidType);
	GoToFluidTypePathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToSpaceDesignationPathRequest : public PathRequestBreadthFirst
{
	SpaceDesignation designation;
	GoToSpaceDesignationPathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination, SpaceDesignation designation);
	GoToSpaceDesignationPathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	[[nodiscard]] Json toJson() const override;
};
struct GoToEdgePathRequest : public PathRequestBreadthFirst
{
	GoToEdgePathRequest(Point3D start, Distance maxRange, ActorReference actor, ShapeId shape, MoveTypeId moveType, Facing4 facing, bool detour, bool adjacent, bool reserveDestination);
	GoToEdgePathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	[[nodiscard]] Json toJson() const override;
};