#pragma once
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/linePath.h"
#include "../geometry/rectangle.h"
class Area;
namespace longRangePath
{
	// Check if a target is somewhere in the provided cuboid.
	template<typename F>
	concept LongRangeCondition = requires(F f, Cuboid cuboid) {
		{ f(cuboid) } -> std::same_as<bool>;
	};
	// Check if a point and facing are a valid destination.
	template<typename F>
	concept ShortRangeCondition = requires(F f, Point3D point, Facing4 facing) {
		{ f(point, facing) } -> std::same_as<Point3D>;
	};
	// Check if a cuboid contains a target and return it.
	template<typename F>
	concept ShortRangeCuboidCondition = requires(F f, Cuboid cuboid) {
		{ f(cuboid) } -> std::same_as<Point3D>;
	};
	// Check if a ShortRangeCondition is valid for either point or cuboid variety.
	template<typename F>
	concept ShortRangeConditionPointOrCuboid = ShortRangeCondition<F> || ShortRangeCuboidCondition<F>;
	struct OpenListData
	{
		Cuboid current;
		Cuboid previous;
		int cost;
		int score;
		bool shortRange;
	};
	// Memos cannot be moved or copied.
	struct ShortRangeMemo
	{
		SmallSet<Point3D> openList;
		CuboidSet closedList;
		ShortRangeMemo() = default;
	};
	struct LongRangeMemo
	{
		ShortRangeMemo shortRangeMemo;
		std::vector<OpenListData> openList;
		SmallMap<Cuboid, SmallMap<Cuboid, int>> closedListToFromWithAccumulatedCost;
		LongRangeMemo() = default;
	};
	inline std::vector<std::unique_ptr<LongRangeMemo>> memos;
	inline std::vector<bool> memoCheckedOut;
	inline std::mutex memoMutex;
	// Entry point.
	// ShortRangeConditionT is not constrained by the ShortRangeCondition concept here because we don't know if it takes a cuboid or a point and facing as argument(s).
	// Wraps condition for unreserved by faction, if any faction is supplied.
	template<LongRangeCondition LongRangeConditionT, ShortRangeConditionPointOrCuboid ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> getPath(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const FactionId faction, const bool depthFirst, const bool singleTile, const bool detour, const bool adjacent, const bool anyOccupied, const Distance maxRange = Distance::null(), const Point3D huristicDestination = {});
	// Maybe apply adjacent or occupied.
	// After this template ShortRangeCondition is always (Point, Facing4) -> Point, not (Cuboid) -> Point.
	template<LongRangeCondition LongRangeConditionT, ShortRangeConditionPointOrCuboid ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> getPathAdjacent(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const bool depthFirst, const bool singleTile, const bool detour, const bool adjacent, const bool anyOccupied, const Distance maxRange, const Point3D huristicDestination);
	// Maybe apply maxRange.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> getPathMaxRange(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const bool depthFirst, const bool singleTile, const bool detour, const bool adjacent, const bool anyOccupied, const Distance maxRange, const Point3D huristicDestination);
	// Returns path and target block which passed condition.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT, bool depthFirst, bool singleTile, bool detour>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> getPathInnerLoop(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, LongRangeMemo& memo, const Point3D huristicDestination = {});
	// A helper for getPath.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT, bool depthFirst, bool singleTile, bool detour>
	[[nodiscard]] std::tuple<SmallSet<Cuboid>, Point3D, Point3D, Point3D> withEachCuboid(const Area& area, const auto& rtree, const Cuboid cuboid, const Cuboid previous, const Cuboid startCuboid, const Point3D start, const ShapeId shape, const int width, const int length, const int height, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const int cost, LongRangeMemo& memo, const Point3D huristicDestination);
	// Find which cuboids adjacent to current can be entered. Short range pathing to be used when Cuboid is not freely navigable or shape enters through a constrained portal.
	template<bool detour>
	[[nodiscard]] SmallSet<Cuboid> getAccessableCuboids(const Area& area, const auto& rtree, const Cuboid current, const Cuboid previous, const ShapeId shape, const CuboidSet& occupied, ShortRangeMemo& memo);
	// Returns destination, point before destinaiton, and target. Returning point before allows the cuboid path to point path converter to path to the point before and then append the destinaiton, thus preserving the facing which was being used when finding the target.
	template<ShortRangeCondition ConditionT, bool detour>
	std::tuple<Point3D, Point3D, Point3D> getAccessableDestination(const Area& area, const auto& rtree, const Cuboid cuboid, const Cuboid previous, const ShapeId shape, ConditionT&& condition, ShortRangeMemo& memo, const CuboidSet& occupied);
	// These three functions dispatch to different template specializations depending on run time bool arguments.
	template<bool depthFirst, bool singleTile, LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D>  getPathDetour(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition,  LongRangeMemo& memo, const bool detour, const Point3D huristicDestination);
	template<bool depthFirst, LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D>  getPathSingleTile(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition,  LongRangeMemo& memo, const bool singleTile, const bool detour, const Point3D huristicDestination);
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D>  getPathDepthFirst(const Area& area, const auto& rtree, const Point3D start, const Facing4 startFacing, const ShapeId shape, const CuboidSet& occupied, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition,  LongRangeMemo& memo, const bool depthFirst, const bool singleTile, const bool detour, const Point3D huristicDestination);
	// Get deflection point for portal.
	[[nodiscard]] Point3D findCrossingPointNearestForShape(Area& area, RectangleWithFacing portal, Point3D huristic, ShapeId shape, ShortRangeMemo& memo);
	// Check if a cuboid is small enough that short range pathing is needed. Checking if shape can turn and if vertical clearence is enough for height everywhere in the cuboid.
	[[nodiscard]] bool checkCuboidNotFreelyNavigable(const Cuboid cuboid, const int width, const int length, const int height);
	// Check if a shape can pass freely between two adjacent cuboids or if we must do a detailed check to see that the shape fits. Return true for freely pathable.
	[[nodiscard]] bool checkPortalSize(const Cuboid to, const Cuboid from, const int width, const int length, const int height);
	// Pass both end and point before end so that point before can be used for building the path and end can be appended to the back, thus preserving the facing of end which was used when the short range condition found the target.
	[[nodiscard]] SmallSet<Point3D> cuboidPathToPointPathStringPulling(Area& area, ShapeId shape, const SmallSet<Cuboid> cuboids, Point3D start, Point3D end, Point3D pointBeforeEnd, ShortRangeMemo& memo);
	// A helper for cuboidPathToPointPathStringPulling.
	[[nodiscard]] SmallSet<Point3D> generateSegmentStringPulling(Area& area, ShapeId shape, Point3D start, Point3D end, std::vector<RectangleWithFacing>::iterator startPortal, std::vector<RectangleWithFacing>::iterator endPortal, ShortRangeMemo& memo);
	// Constructs whole path point by point while using cuboids to constrain search space.
	// TODO: would it be better to just make the detour path without involving cuboids / long range at all?
	template<bool detour>
	[[nodiscard]] SmallSet<Point3D> cuboidPathToPointPath(Area& area, ShapeId shape, const CuboidSet& occupied, const SmallSet<Cuboid> cuboids, Point3D start, Point3D end, Point3D pointBeforeEnd, ShortRangeMemo& memo);
	// Manage memos, thread safe.
	[[nodiscard]] LongRangeMemo& checkOutMemo();
	void returnMemo(LongRangeMemo& memo);
}