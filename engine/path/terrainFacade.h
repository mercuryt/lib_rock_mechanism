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
#include "pathRequest.h"
#include "reference.h"
#include "numericTypes/types.h"
#include "callbackTypes.h"
#include "dataStructures/strongVector.h"
#include "dataStructures/smallMap.h"
#include "dataStructures/rtreeData.h"

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
	PathRequestNoHuristicData(std::unique_ptr<PathRequestBreadthFirst> pr, uint32_t sh);
	PathRequestNoHuristicData(const PathRequestNoHuristicData& other) = delete;
	PathRequestNoHuristicData(PathRequestNoHuristicData&& other) noexcept;
	PathRequestNoHuristicData& operator=(const PathRequestNoHuristicData& other) = delete;
	PathRequestNoHuristicData& operator=(PathRequestNoHuristicData&& other) noexcept;
};
struct PathRequestWithHuristicData
{
	FindPathResult result;
	std::unique_ptr<PathRequestDepthFirst> pathRequest;
	uint32_t spatialHash;
	PathRequestWithHuristicData() = default;
	PathRequestWithHuristicData(std::unique_ptr<PathRequestDepthFirst> pr, uint32_t sh);
	PathRequestWithHuristicData(const PathRequestWithHuristicData& other) = delete;
	PathRequestWithHuristicData(PathRequestWithHuristicData&& other) noexcept;
	PathRequestWithHuristicData& operator=(const PathRequestWithHuristicData& other) = delete;
	PathRequestWithHuristicData& operator=(PathRequestWithHuristicData&& other) noexcept;
};
struct AdjacentData
{
	using Primitive = uint32_t;
	std::bitset<maxAdjacent> data;
	void set(const AdjacentIndex& index, bool value) { data[index.get()] = value; }
	[[nodiscard]] bool check(const AdjacentIndex& index) const { return data[index.get()]; }
	[[nodiscard]] Primitive get() const { return data.to_ulong(); }
	[[nodiscard]] auto operator<=>(const AdjacentData& other) const { return data.to_ulong() <=> other.data.to_ulong(); }
	[[nodiscard]] bool operator==(const AdjacentData& other) const { return data.to_ulong() == other.data.to_ulong(); }
	[[nodiscard]] bool empty() const { return !data.any(); }
	[[nodiscard]] std::string toString() const { return data.to_string(); }
	static AdjacentData create(const Primitive& d) { return {d}; }
	static AdjacentData null() { return {0}; }
	class ConstIterator
	{
		std::bitset<maxAdjacent> data;
		AdjacentIndex index;
		void advanceToNext() { while(index < maxAdjacent && !data[index.get()]) ++index; }
	public:
		ConstIterator(const std::bitset<maxAdjacent>& d, const AdjacentIndex& i) : data(d), index(i) { advanceToNext(); }
		[[nodiscard]] const AdjacentIndex& operator*() const { return index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) { return index != other.index; }
		void operator++() { ++index; advanceToNext(); }
	};
	[[nodiscard]] ConstIterator begin() const { return {data, AdjacentIndex::create(0)}; }
	[[nodiscard]] ConstIterator end() const { return {data, AdjacentIndex::create(maxAdjacent)}; }
};
class TerrainFacade final
{
	StrongVector<PathRequestNoHuristicData, PathRequestIndex> m_pathRequestsNoHuristic;
	StrongVector<PathRequestWithHuristicData, PathRequestIndex> m_pathRequestsWithHuristic;
	RTreeData<AdjacentData> m_enterable;
	Area& m_area;
	Point3D m_pointToIndexConversionMultipliers;
	MoveTypeId m_moveType;
	// Non batched pathing uses the WithoutMemo variants.
	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathBreadthFirstWithoutMemo(const Point3D& start, const Facing4& startFacing, DestinationConditionT& destinationCondition, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const;
	template<bool anyOccupiedPoint, DestinationCondition DestinationConditionT>
	[[nodiscard]] FindPathResult findPathDepthFirstWithoutMemo(const Point3D& from, const Facing4& startFacing, DestinationConditionT& destinationCondition, const Point3D& huristicDestination, const ShapeId& shape, const MoveTypeId& moveType, bool detour, bool adjacent, const FactionId& faction, const Distance& maxRange) const;
	[[nodiscard]] PathRequestIndex getPathRequestIndexNoHuristic();
	[[nodiscard]] PathRequestIndex getPathRequestIndexWithHuristic();
	[[nodiscard]] AdjacentData makeDataForPoint(const Point3D& point) const;
	void movePathRequestNoHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
	void movePathRequestWithHuristic(PathRequestIndex oldIndex, PathRequestIndex newIndex);
	void updatePoint(const Point3D& point);
public:
	TerrainFacade(Area& area, const MoveTypeId& moveType);
	TerrainFacade(TerrainFacade&&) noexcept = default;
	TerrainFacade& operator=(TerrainFacade&&) noexcept = default;
	void doStep();
	void registerPathRequestNoHuristic(std::unique_ptr<PathRequestBreadthFirst> pathRequest);
	void registerPathRequestWithHuristic(std::unique_ptr<PathRequestDepthFirst> pathRequest);
	void unregisterNoHuristic(PathRequest& pathRequest);
	void unregisterWithHuristic(PathRequest& pathRequest);
	void update(const Cuboid& cuboid);
	void maybeSetImpassable(const Cuboid& cuboid);
	void clearPathRequests();
	[[nodiscard]] const AdjacentData getAdjacentData(const Point3D& point) const { return m_enterable.queryGetOne(point); }
	[[nodiscard]] bool getValue(const Point3D& to, const Point3D& from) const;
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
	[[nodiscard]] auto& getArea() { return m_area; }
	[[nodiscard]] const auto& getArea() const { return m_area; }
	// For testing.
	[[nodiscard]] size_t getPathRequestCount() const { return m_pathRequestsNoHuristic.size() + m_pathRequestsWithHuristic.size(); }
	[[nodiscard]] bool accessable(const Point3D& from, const Facing4& startFacing, const Point3D& to, const ActorIndex& actor) const;
	[[nodiscard]] bool empty() const { return m_pathRequestsNoHuristic.empty() && m_pathRequestsWithHuristic.empty(); }
	[[nodiscard]] CuboidSet getEnterableIntersection(const auto& shape) const { return m_enterable.queryGetIntersection(shape); }
};
class AreaHasTerrainFacades
{
	SmallMap<MoveTypeId, TerrainFacade> m_data;
	Area& m_area;
public:
	AreaHasTerrainFacades(Area& area) : m_area(area) { }
	void doStep();
	void maybeRegisterMoveType(const MoveTypeId& moveType);
	void clearPathRequests();
	void update(const Cuboid& cuboid);
	void maybeSetImpassable(const Cuboid& cuboid);
	[[nodiscard]] TerrainFacade& getForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] TerrainFacade& getOrCreateForMoveType(const MoveTypeId& moveTypeId);
	[[nodiscard]] bool empty() const { for(const auto& pair : m_data) { if(!pair.second.empty()) return false; } return true; }
	[[nodiscard]] bool contains(const MoveTypeId& moveTypeId) const { return m_data.contains(moveTypeId); }
};