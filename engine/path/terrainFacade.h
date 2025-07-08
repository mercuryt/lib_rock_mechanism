/*
 * TerrainFacade:
 * Stores enterability data for a specific move type for pair of space in a binary format.
 * Also stores path requests in a series of vectors divided by presence or absence of destination huristic.
 * This divide is done for instruction cache locality.
 * TODO: try replacing std::function accessCondition and destinationCondition with a single findPath std::function member which calls a static class method in the class derived from PathRequest which calls a findPath template function.
 *
 */
#pragma once

#include "config.h"
#include "designations.h"
#include "numericTypes/index.h"
#include "pathMemo.h"
#include "reference.h"
#include "numericTypes/types.h"
#include "callbackTypes.h"
#include "dataStructures/strongVector.h"
#include "dataStructures/smallMap.h"
#include "dataStructures/rtreeDataIndex.h"

#include <bitset>

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
// TODO: optimization: single point shapes don't need facing

struct FindPathResult
{
	SmallSet<Point3D> path;
	Point3D pointThatPassedPredicate;
	bool useCurrentPosition = false;
	FindPathResult() = default;
	FindPathResult(const SmallSet<Point3D>& p, Point3D btpp, bool ucp);
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
	// TODO: This should not be using the Index variant but the Data variant requires a "Primitive" class member type. Either remove Primitive from RTreeData or make a wrapper for bitset.
	// TODO: can this index be 16 bit?
	RTreeDataIndex<std::bitset<26>, uint32_t> m_enterable;
	Area& m_area;
	Point3D m_pointToIndexConversionMultipliers;
	MoveTypeId m_moveType;
	// Non batched pathing uses the WithoutMemo variants.
	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathBreadthFirstWithoutMemo(const Point3D& start, const Facing4& startFacing, DestinationConditionT& destinationCondition, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const;
	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathDepthFirstWithoutMemo(const Point3D& from, const Facing4& startFacing, DestinationConditionT& destinationCondition, const Point3D& huristicDestination, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const;
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
	void update(const Point3D& point);
	void clearPathRequests();
	[[nodiscard]] bool canEnterFrom(const Point3D &point, AdjacentIndex adjacentIndex) const;
	[[nodiscard]] bool getValueForBit(const Point3D& from, const Point3D& to) const;
	[[nodiscard]] FindPathResult findPathTo(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null()) const;
	[[nodiscard]] FindPathResult findPathToWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& target, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null()) const;
	[[nodiscard]] FindPathResult findPathToAnyOf(PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, SmallSet<Point3D> indecies, const Point3D huristicDestination, bool detour = false, const FactionId& faction = FactionId::null()) const;
	[[nodiscard]] FindPathResult findPathToAnyOfWithoutMemo(const Point3D& start, const Facing4& startFacing, const ShapeId& shape, SmallSet<Point3D> indecies, const Point3D huristicDestination, bool detour = false, const FactionId& faction = FactionId::null()) const;

	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionDepthFirst(DestinationConditionT& destinationCondition, PathMemoDepthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& huristicDestination, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const Distance& maxRange = Distance::max()) const;

	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionDepthFirstWithoutMemo(DestinationConditionT& destinationCondition, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, const Point3D& huristicDestination, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const Distance& maxRange = Distance::max()) const;

	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionBreadthFirst(DestinationConditionT destinationCondition, PathMemoBreadthFirst& memo, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const Distance& maxRange = Distance::max()) const;

	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToConditionBreadthFirstWithoutMemo(DestinationConditionT& destinationCondition, const Point3D& start, const Facing4& startFacing, const ShapeId& shape, bool detour = false, bool adjacent = false, const FactionId& faction = FactionId::null(), const Distance& maxRange = Distance::max()) const;

	[[nodiscard]] FindPathResult findPathToSpaceDesignation(PathMemoBreadthFirst &memo, const SpaceDesignation designation, const FactionId &faction, const Point3D &start, const Facing4 &startFacing, const ShapeId &shape, bool detour, bool adjacent, bool unreserved, const Distance &maxRange) const;

	// Faction is sent by value on pupose.
	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathToSpaceDesignationAndCondition(DestinationConditionT& destinationCondition, PathMemoBreadthFirst &memo, const SpaceDesignation designation, FactionId faction, const Point3D &start, const Facing4 &startFacing, const ShapeId &shape, bool detour, bool adjacent, bool unreserved, const Distance &maxRange) const;

	[[nodiscard]] FindPathResult findPathToEdge(PathMemoBreadthFirst& memo, const Point3D &start, const Facing4 &startFacing, const ShapeId &shape, bool detour = false) const;
	[[nodiscard]] FindPathResult findPathAdjacentToPolymorphicWithoutMemo(const Point3D &start, const Facing4 &startFacing, const ShapeId &shape, const ActorOrItemIndex &actorOrItem, bool detour = false) const;
	// For testing.
	[[nodiscard]] size_t getPathRequestCount() const { return m_pathRequestsNoHuristic.size() + m_pathRequestsWithHuristic.size(); }
	[[nodiscard]] bool accessable(const Point3D& from, const Facing4& startFacing, const Point3D& to, const ActorIndex& actor) const;
	[[nodiscard]] bool empty() const { return m_pathRequestsNoHuristic.empty() && m_pathRequestsWithHuristic.empty(); }
};
class AreaHasTerrainFacades
{
	SmallMapStable<MoveTypeId, TerrainFacade> m_data;
	Area& m_area;
	void update(const Point3D& point);
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void doStep();
	void updatePointAndAdjacent(const Point3D& point);
	void maybeRegisterMoveType(const MoveTypeId& moveType);
	void clearPathRequests();
	[[nodiscard]] TerrainFacade& getForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] TerrainFacade& getOrCreateForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] bool empty() const { for(const auto& pair : m_data) { if(!pair.second->empty()) return false; } return true; }
	[[nodiscard]] bool contains(const MoveTypeId& moveTypeId) const { return m_data.contains(moveTypeId); }
};