#pragma once

#include "types.h"
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
	BlockIndex start;
	DistanceInBlocks maxRange;
	ActorReference actor;
	ShapeId shape;
	FactionId faction;
	MoveTypeId moveType;
	Facing facing;
	bool detour;
	bool adjacent;
	bool reserveDestination;
	PathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination);
	PathRequest() = default;
	PathRequest(const Json& data, Area& area);
	[[nodiscard]] static PathRequest& load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const MoveTypeId& moveType);
	[[nodiscard]] static PathRequest& record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestBreadthFirst> pathRequest);
	[[nodiscard]] static PathRequest& record(Area &area, const MoveTypeId& moveType, std::unique_ptr<PathRequestDepthFirst> pathRequest);
	virtual void writeStep(Area& area, FindPathResult& result);
	virtual void cancel(Area& area) = 0;
	virtual ~PathRequest() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(PathRequest, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination);
};
struct PathRequestDepthFirst : public PathRequest
{
	BlockIndex huristicDestination;
	PathRequestDepthFirst(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockIndex huristicDestination);
	PathRequestDepthFirst() = default;
	PathRequestDepthFirst(const Json& data, Area& area);
	void cancel(Area& area) override;
	void record(Area& area, std::unique_ptr<PathRequestDepthFirst>& pointerToThis);
	virtual FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) = 0;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(PathRequestDepthFirst, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, huristicDestination);
};
struct PathRequestBreadthFirst : public PathRequest
{
	PathRequestBreadthFirst(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination);
	PathRequestBreadthFirst() = default;
	PathRequestBreadthFirst(const Json& data, Area& area);
	void cancel(Area& area) override;
	void record(Area& area, std::unique_ptr<PathRequestBreadthFirst>& pointerToThis);
	virtual FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) = 0;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(PathRequestBreadthFirst, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination);
};
struct GoToPathRequest : public PathRequestDepthFirst
{
	BlockIndex destination;
	GoToPathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockIndex destination);
	GoToPathRequest(const Json& data, Area& area);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(GoToPathRequest, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, huristicDestination, destination);
};
struct GoToAnyPathRequest : public PathRequestDepthFirst
{
	BlockIndices destinations;
	GoToAnyPathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockIndex huristicDestination, BlockIndices destinations);
	GoToAnyPathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(GoToAnyPathRequest, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, huristicDestination, destinations);
};
struct GoToFluidTypePathRequest : public PathRequestBreadthFirst
{
	FluidTypeId fluidType;
	GoToFluidTypePathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, FluidTypeId fluidType);
	GoToFluidTypePathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(GoToFluidTypePathRequest, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, fluidType);
};
struct GoToBlockDesignationPathRequest : public PathRequestBreadthFirst
{
	BlockDesignation designation;
	GoToBlockDesignationPathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, FactionId faction, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination, BlockDesignation designation); 
	GoToBlockDesignationPathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(GoToBlockDesignationPathRequest, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination, designation);
};
struct GoToEdgePathRequest : public PathRequestBreadthFirst
{
	GoToEdgePathRequest(BlockIndex start, DistanceInBlocks maxRange, ActorReference actor, ShapeId shape, MoveTypeId moveType, Facing facing, bool detour, bool adjacent, bool reserveDestination); 
	GoToEdgePathRequest(const Json& data, Area& area);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(GoToEdgePathRequest, start, maxRange, actor, shape, faction, moveType, facing, detour, adjacent, reserveDestination);
};