#pragma once
#include "../numericTypes/idTypes.h"
#include "../dataStructures/rtreeBoolean.h"
#include "../dataStructures/smallSet.h"
#include "longRange.h"
#include "enterable.h"
struct PathRequest;
class Area;

struct AreaHasPathsForMoveType
{
	AreaHasPathsForMoveType(Area& area, const MoveTypeId moveType);
	Enterable m_enterable;
	MoveTypeId m_moveType;
	std::vector<std::unique_ptr<PathRequest>> m_pathRequests;
	void readStepForRequest(Area& area, PathRequest& pathRequest);
	void writeStep(Area& area);
	void recordPathRequest(std::unique_ptr<PathRequest> pathRequest);
	void cancelPathRequest(PathRequest& pathRequest);
	void update(Area& area, const Cuboid cuboid);
	[[nodiscard]] PathResult pathTo(const PathParamaters& params) const;
	[[nodiscard]] PathResult pathToEdge(const PathParamaters& params) const;
	[[nodiscard]] PathResult pathToDesignation(SpaceDesignation designation, const PathParamaters& params) const;
	[[nodiscard]] bool accessable(const PathParamaters& params) const;
	template<bool adjacent, bool anyOccupied, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
	[[nodiscard]] Point3D accessableCondition(LongRangeConditionT& longRangeCondition, ShortRangeConditionT& shortRangeCondition, const PathParamaters& params) const;
	template<bool adjacent, bool anyOccupied, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
	[[nodiscard]] PathResult pathToCondition(LongRangeConditionT& longRangeCondition, ShortRangeConditionT& shortRangeCondition, const PathParamaters& params) const;
	template<bool adjacent, bool anyOccupiedPoint, longRangePath::LongRangeCondition LongRangeConditionT, longRangePath::ShortRangeConditionPointOrCuboid ShortRangeConditionT>
	[[nodiscard]] PathResult pathToConditionWithDesignation(SpaceDesignation designation, LongRangeConditionT& longRangeCondition, ShortRangeConditionT& shortRangeCondition, const PathParamaters& params) const;
};
class AreaHasPaths
{
	std::vector<AreaHasPathsForMoveType> m_data;
	std::vector<std::pair<int, int>> m_outerAndInnerIndices;
public:
	void doStep(Area& area);
	void registerMoveType(Area& area, const MoveTypeId id);
	void maybeRegisterMoveType(Area& area, const MoveTypeId id);
	void clearPathRequests();
	void update(Area& area, const Cuboid cuboid);
	void maybeSetImpassable(const Cuboid cuboid);
	[[nodiscard]] AreaHasPathsForMoveType& get(const MoveTypeId id);
	[[nodiscard]] bool empty() const { return m_data.empty(); }
};