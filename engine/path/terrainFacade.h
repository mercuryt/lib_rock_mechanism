/*
 * TerrainFacade:
 * Stores enterability data for a specific move type for pair of blocks in a binary format.
 * Also stores path requests in a series of vectors divided by presence or absence of destination huristic.
 * This divide is done for instruction cache locality.
 * TODO: try replacing std::function accessCondition and destinationCondition with a single findPath std::function member which calls a static class method in the class derived from PathRequest which calls a findPath template function.
 *
 */
#pragma once

#include "config.h"
#include "designations.h"
#include "index.h"
#include "pathMemo.h"
#include "reference.h"
#include "types.h"
#include "callbackTypes.h"
#include "dataVector.h"

#include <pthread.h>

struct MoveType;
struct Shape;
struct FluidType;
class Area;
class TerrainFacade;
class ActorOrItemIndex;
class PathRequest;
class PathMemoBreadthFirst;
class PathMemoDepthFirst;
class TerrainFacade;
class PathRequestBreadthFirst;
class PathRequestDepthFirst;

constexpr int maxAdjacent = 26;
// TODO: optimization: single block shapes don't need facing

struct FindPathResult
{
	SmallSet<BlockIndex> path;
	BlockIndex blockThatPassedPredicate;
	bool useCurrentPosition = false;
	FindPathResult() = default;
	FindPathResult(const SmallSet<BlockIndex>& p, BlockIndex btpp, bool ucp);
	void validate() const;
	FindPathResult(const FindPathResult& other) noexcept = default;
	FindPathResult(FindPathResult&& other) noexcept = default;
	FindPathResult& operator=(FindPathResult&& other) noexcept = default;
	FindPathResult& operator=(const FindPathResult& other) noexcept = default;
};
struct PathRequestNoHuristicData
{
	FindPathResult result;
	std::unique_ptr<PathRequestBreadthFirst> pathRequest;
	uint32_t spatialHash;
	PathRequestNoHuristicData() = default;
	PathRequestNoHuristicData(std::unique_ptr<PathRequestBreadthFirst> pr, uint32_t sh) :
		pathRequest(std::move(pr)),
		spatialHash(sh)
	{ }
	PathRequestNoHuristicData(const PathRequestNoHuristicData& other) = delete;
	PathRequestNoHuristicData(PathRequestNoHuristicData&& other) noexcept :
		pathRequest(std::move(other.pathRequest)),
		spatialHash(other.spatialHash)
	{ }
	PathRequestNoHuristicData& operator=(const PathRequestNoHuristicData& other) = delete;
	PathRequestNoHuristicData& operator=(PathRequestNoHuristicData&& other) noexcept
	{
		pathRequest = std::move(other.pathRequest);
		spatialHash = other.spatialHash;
		return *this;
	}
};
struct PathRequestWithHuristicData
{
	FindPathResult result;
	std::unique_ptr<PathRequestDepthFirst> pathRequest;
	uint32_t spatialHash;
	PathRequestWithHuristicData() = default;
	PathRequestWithHuristicData(std::unique_ptr<PathRequestDepthFirst> pr, uint32_t sh) :
		pathRequest(std::move(pr)),
		spatialHash(sh)
	{ }
	PathRequestWithHuristicData(const PathRequestWithHuristicData& other) = delete;
	PathRequestWithHuristicData(PathRequestWithHuristicData&& other) noexcept :
		pathRequest(std::move(other.pathRequest)),
		spatialHash(other.spatialHash)
	{ }
	PathRequestWithHuristicData& operator=(const PathRequestWithHuristicData& other) = delete;
	PathRequestWithHuristicData& operator=(PathRequestWithHuristicData&& other) noexcept
	{
		pathRequest = std::move(other.pathRequest);
		spatialHash = other.spatialHash;
		return *this;
	}
};
class TerrainFacade final
{
	StrongVector<PathRequestNoHuristicData, PathRequestIndex> m_pathRequestsNoHuristic;
	StrongVector<PathRequestWithHuristicData, PathRequestIndex> m_pathRequestsWithHuristic;
	// Not indexed by BlockIndex because size is multipied by max adjacent.
	std::vector<bool> m_enterable;
	Area& m_area;
	MoveTypeId m_moveType;
	// Non batched pathing uses the WithoutMemo variants.
	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathBreadthFirstWithoutMemo(const BlockIndex& start, const Facing4& startFacing, DestinationConditionT& destinationCondition, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const;
	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathDepthFirstWithoutMemo(const BlockIndex& from, const Facing4& startFacing, DestinationConditionT& destinationCondition, const BlockIndex& huristicDestination, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const DistanceInBlocks& maxRange) const;
	PathRequestIndex getPathRequestIndexNoHuristic();
	PathRequestIndex getPathRequestIndexWithHuristic();
	void movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
	void movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
public:
	TerrainFacade(Area& area, const MoveTypeId& moveType);
	void doStep();
	void registerPathRequestNoHuristic(std::unique_ptr<PathRequestBreadthFirst> pathRequest);
	void registerPathRequestWithHuristic(std::unique_ptr<PathRequestDepthFirst> pathRequest);
	void unregisterNoHuristic(PathRequest& pathRequest);
	void unregisterWithHuristic(PathRequest& pathRequest);
	void update(const BlockIndex& block);
	void clearPathRequests();
	[[nodiscard]] bool canEnterFrom(const BlockIndex& blockIndex, AdjacentIndex adjacentIndex) const;
	[[nodiscard]] bool getValueForBit(const BlockIndex& from, const BlockIndex& to) const;
	[[nodiscard]] FindPathResult findPathTo(PathMemoDepthFirst& memo, const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null()) const;
	[[nodiscard]] FindPathResult findPathToWithoutMemo(const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, const BlockIndex& target, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null()) const;
	[[nodiscard]] FindPathResult findPathToAnyOf(PathMemoDepthFirst& memo, const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, SmallSet<BlockIndex> indecies, const BlockIndex huristicDestination, bool detour = false, const FactionId& faction = FactionId::null()) const;
	[[nodiscard]] FindPathResult findPathToAnyOfWithoutMemo(const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, SmallSet<BlockIndex> indecies, const BlockIndex huristicDestination, bool detour = false, const FactionId& faction = FactionId::null()) const;

	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionDepthFirst(DestinationConditionT& destinationCondition, PathMemoDepthFirst& memo, const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, const BlockIndex& huristicDestination, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const DistanceInBlocks& maxRange = DistanceInBlocks::max()) const;

	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionDepthFirstWithoutMemo(DestinationConditionT& destinationCondition, const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, const BlockIndex& huristicDestination, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const DistanceInBlocks& maxRange = DistanceInBlocks::max()) const;

	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionBreadthFirst(DestinationConditionT destinationCondition, PathMemoBreadthFirst& memo, const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const DistanceInBlocks& maxRange = DistanceInBlocks::max()) const;

	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionBreadthFirstWithoutMemo(DestinationConditionT& destinationCondition, const BlockIndex& start, const Facing4& startFacing, const ShapeId& shape, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const DistanceInBlocks& maxRange = DistanceInBlocks::max()) const;

	[[nodiscard]] FindPathResult findPathToBlockDesignation(PathMemoBreadthFirst &memo, const BlockDesignation designation, const FactionId &faction, const BlockIndex &start, const Facing4 &startFacing, const ShapeId &shape, bool detour, bool adjacent, bool unreserved, const DistanceInBlocks &maxRange) const;

	// Faction is sent by value on pupose.
	template<bool anyOccupiedBlock, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToBlockDesignationAndCondition(DestinationConditionT& destinationCondition, PathMemoBreadthFirst &memo, const BlockDesignation designation, FactionId faction, const BlockIndex &start, const Facing4 &startFacing, const ShapeId &shape, bool detour, bool adjacent, bool unreserved, const DistanceInBlocks &maxRange) const;

	[[nodiscard]] FindPathResult findPathToEdge(PathMemoBreadthFirst& memo, const BlockIndex &start, const Facing4 &startFacing, const ShapeId &shape, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToPolymorphicWithoutMemo(const BlockIndex &start, const Facing4 &startFacing, const ShapeId &shape, const ActorOrItemIndex &actorOrItem, bool detour = false) const;
	// For testing.
	[[nodiscard]] size_t getPathRequestCount() const { return m_pathRequestsNoHuristic.size() + m_pathRequestsWithHuristic.size(); }
	[[nodiscard]] bool accessable(const BlockIndex& from, const Facing4& startFacing, const BlockIndex& to, const ActorIndex& actor) const;
	[[nodiscard]] bool empty() const { return m_pathRequestsNoHuristic.empty() && m_pathRequestsWithHuristic.empty(); }
};
class AreaHasTerrainFacades
{
	SmallMapStable<MoveTypeId, TerrainFacade> m_data;
	Area& m_area;
	void update(const BlockIndex& block);
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void doStep();
	void updateBlockAndAdjacent(const BlockIndex& block);
	void maybeRegisterMoveType(const MoveTypeId& moveType);
	void clearPathRequests();
	[[nodiscard]] TerrainFacade& getForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] TerrainFacade& getOrCreateForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] bool empty() const { for(const auto& pair : m_data) { if(!pair.second->empty()) return false; } return true; }
	[[nodiscard]] bool contains(const MoveTypeId& moveTypeId) const { return m_data.contains(moveTypeId); }
};