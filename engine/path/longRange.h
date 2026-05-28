/*
	getPath->getPathUnreserved->getPathMaxRange->getPathDepthFirst->getPathSingleTile->getPathDetour->getPathInnerLoop
	getPathAdjacent->getPath
	getPathAnyOccupied->getPath
*/
#pragma once
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/linePath.h"
#include "../geometry/rectangle.h"
#include "../dataStructures/smallMap.h"
#include "result.h"
#include "paramaters.h"
#include "closedListWithFacing.h"
#include "closedListLongRange.h"
#include <omp.h>
class Area;
class Enterable;
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
		std::vector<Point3D> openList;
		std::vector<std::pair<Point3D, int>> openListWithCuboidIndex;
		CuboidSet closedList;
		std::vector<std::pair<Point3D, Point3D>> history;
		ClosedListWithFacing closedListWithFacing;
		ShortRangeMemo() = default;
	};
	// Four cache lines per memo to prevent false sharing.
	struct alignas(std::hardware_destructive_interference_size * 4) LongRangeMemo
	{
		ShortRangeMemo shortRangeMemo;
		std::vector<OpenListData> openList;
		ClosedListLongRange closedListToFrom;
		LongRangeMemo() = default;
	};
	// This vector is allocated during initalization and then never changed. Addresses within it are stable.
	inline std::vector<LongRangeMemo> memos;
	void init();
	inline LongRangeMemo& getMemo() { return memos[omp_get_thread_num()]; }
	// Pass IntermediateResult out from inner templates to make testing easier.
	struct IntermediateResult
	{
		std::vector<Cuboid> cuboids;
		Point3D target;
	};
	// Entry point.
	// Convinence wrapper to convert adjacent condition to location + facing condition.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCuboidCondition ShortRangeConditionT>
	[[nodiscard]] PathResult getPathAdjacent(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params);
	// Entry point.
	// Convinence wrapper to convert anyOccupied condition to location + facing condition.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCuboidCondition ShortRangeConditionT>
	[[nodiscard]] PathResult getPathAnyOccupied(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params);
	// Entry point.
	// Destination is passed as PathParamaters::huristicDestination
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] PathResult getPath(const Enterable& rtree, LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params);
	// Wraps condition for unreserved by faction, if any faction is supplied.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] IntermediateResult getPathUnreserved(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params);
	// Maybe apply maxRange.
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] IntermediateResult getPathMaxRange(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params);
	// Returns path and target block which passed condition.
	template<bool depthFirst, bool singleTile, bool detour, LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] IntermediateResult getPathInnerLoop(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params);
	// A helper for getPath.
	template<bool depthFirst, bool singleTile, bool detour>
	void withEachAdjacentCuboid(const Cuboid cuboid, const Cuboid previous, const int cost, const bool useShortRange, LongRangeMemo& memo, const PathParamaters& params);
	// A helper for getPath.
	// Find which cuboids adjacent to current can be entered. Short range pathing to be used when Cuboid is not freely navigable or shape enters through a constrained portal.
	template<bool singleTile, bool detour>
	[[nodiscard]] SmallMap<Cuboid, bool> getAccessableCuboidsShortRange(const Enterable& rtree,const Cuboid current, const Cuboid previous, const PathParamaters& params, ShortRangeMemo& memo);
	template<bool singleTile, bool detour>
	[[nodiscard]] SmallMap<Cuboid, bool> getAccessableCuboidsLongRange(const Enterable& rtree,const Cuboid current, const PathParamaters& params);
	// A helper for getPath.
	// Returns target to be used as huristic destination when generating the final path.
	template<bool singleTile, bool detour, ShortRangeCondition ConditionT>
	Point3D getAccessableDestination(const Cuboid cuboid, const Cuboid previous, ConditionT&& condition, ShortRangeMemo& memo, const PathParamaters& params);
	// These three functions dispatch to different template specializations depending on run time bool arguments.
	template<bool depthFirst, bool singleTile, LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] IntermediateResult getPathDetour(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& paramaters);
	template<bool depthFirst, LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] IntermediateResult getPathSingleTile(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& paramaters);
	template<LongRangeCondition LongRangeConditionT, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] IntermediateResult getPathDepthFirst(const Enterable& rtree,LongRangeConditionT&& longRangeCondition, ShortRangeConditionT&& shortRangeCondition, const PathParamaters& paramaters);
	// Get deflection point for portal.
	[[nodiscard]] Point3D findCrossingPointNearestForShape(const Area& area, RectangleWithFacing portal, Point3D huristic, ShapeId shape, ShortRangeMemo& memo);
	// Check if a cuboid is small enough that short range pathing is needed. Checking if shape can turn and if vertical clearence is enough for height everywhere in the cuboid.
	[[nodiscard]] bool checkCuboidNotFreelyNavigable(const Enterable& enterable, const Cuboid cuboid, const PathParamaters& params);
	// Check if a shape can pass freely between two adjacent cuboids or if we must do a detailed check to see that the shape fits. Return true for freely pathable.
	// May return a false negitive.
	[[nodiscard]] bool checkPortalSize(const Cuboid to, const Cuboid from, const PathParamaters& params);
	// If check portal size returns false then we go on to checkPortalPathability to confirm.
	template<bool detour, bool singleTile>
	[[nodiscard]] bool checkPortalPathabality(const Cuboid to, const Cuboid from, const PathParamaters& params);
	// Pass both end and point before end so that point before can be used for building the path and end can be appended to the back, thus preserving the facing of end which was used when the short range condition found the target.
	[[nodiscard]] SmallSet<Point3D> cuboidPathToPointPathStringPulling(const Area& area, ShapeId shape, const std::vector<Cuboid> cuboids, Point3D start, Point3D end, Point3D pointBeforeEnd, ShortRangeMemo& memo);
	// A helper for cuboidPathToPointPathStringPulling.
	[[nodiscard]] SmallSet<Point3D> generateSegmentStringPulling(const Area& area, ShapeId shape, Point3D start, Point3D end, std::vector<RectangleWithFacing>::iterator startPortal, std::vector<RectangleWithFacing>::iterator endPortal, ShortRangeMemo& memo);
	// Constructs whole path point by point while using cuboids to constrain search space.
	// Returns path and target which passed short range condition.
	template<bool detour, ShortRangeCondition ShortRangeConditionT>
	[[nodiscard]] std::pair<SmallSet<Point3D>, Point3D> cuboidPathToPointPath(ShortRangeConditionT&& shortRangeCondition, const PathParamaters& params, const IntermediateResult& intermediateResult, ShortRangeMemo& memo);
	template<bool detour, bool singleTile>
	[[nodiscard]] bool shapeCanEnterFrom(const Point3D to, const Point3D from, const PathParamaters& params);
	template<bool detour, bool singleTile>
	[[nodiscard]] bool shapeCanEnterWithFacing(const Point3D to, const Facing4 facing, const PathParamaters& params);
}