#pragma once
#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include "../dataStructures/smallSet.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/rtreeData.h"
#include "../callbackTypes.h"
#include "../designations.h"

class Space;

struct LongRangePathNode
{
	CuboidSetWithBoundingBoxAdjacent contents;
	SmallSet<LongRangePathNodeIndex> adjacent;
	SmallMap<FactionId, SmallMap<SpaceDesignation, Quantity>> designations;
	bool destroy = false;
	LongRangePathNode() = default;
	LongRangePathNode(const Cuboid& cuboid) { contents.addAndExtend(cuboid); }
	LongRangePathNode(const LongRangePathNode& other) : contents(other.contents), adjacent(other.adjacent), designations(other.designations), destroy(other.destroy) { }
	LongRangePathNode& operator=(const LongRangePathNode& other)
	{
		contents = other.contents;
		adjacent = other.adjacent;
		designations = other.designations;
		destroy = other.destroy;
		return *this;
	}
};
class LongRangePathFinderForMoveType
{
	StrongVector<LongRangePathNode, LongRangePathNodeIndex> m_data;
	RTreeData<LongRangePathNodeIndex> m_pathNodesByPoint;
	void merge(const LongRangePathNodeIndex& mergeInto, const LongRangePathNodeIndex& mergeFrom);
	LongRangePathNodeIndex create(const Point3D& point);
	LongRangePathNodeIndex create(const Cuboid& cuboid);
	void addPointToNode(const LongRangePathNodeIndex& node, const Point3D& point);
public:
	void setPointPathable(Area& area, const Point3D& point);
	void setPointNotPathable(Area& area, const Point3D& point);
	void clearDestroyed();
	void recordPointHasDesignation(Area& area, const Point3D& point, const SpaceDesignation& designation);
	void recordPointDoesNotHaveDesignation(Area& area, const Point3D& point, const SpaceDesignation& designation);
	[[nodiscard]] bool canAbsorbNode(const LongRangePathNode& a, LongRangePathNode& b);
	[[nodiscard]] bool canAbsorbPoint(const LongRangePathNode& node, const Point3D& point);
	[[nodiscard]] bool canAbsorbCuboid(const LongRangePathNode& node, const Cuboid& cuboid);
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToPoint(const Point3D& start, const Point3D& destination);
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToDesignation(const Point3D& start, const SpaceDesignation& designation, const FactionId& faction);
	template<DestinationCondition Condition>
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToPointCondition(const Space& space, const Point3D& start, Condition&& condition)
	{
		auto nodeCondition = [&](const LongRangePathNode& node) -> bool {
			for(const Point3D& point : node.contents.getView())
				// Ignore facing.
				if(condition(point, Facing4::North))
					return true;
			return false;
		};
		return pathToCondition(start, nodeCondition);
	}
	template<typename Condition>
	[[nodiscard]] SmallSet<LongRangePathNodeIndex> pathToCondition(const Point3D& start, Condition&& condition)
	{
		SmallMap<LongRangePathNodeIndex, LongRangePathNodeIndex> closed;
		std::deque<LongRangePathNodeIndex> open;
		const LongRangePathNodeIndex startNodeIndex = m_pathNodesByPoint.queryGetOne(start);
		closed.insert(startNodeIndex, LongRangePathNodeIndex::max());
		open.push_back(startNodeIndex);
		while(!open.empty())
		{
			LongRangePathNodeIndex current = open.front();
			open.pop_front();
			if(condition(current))
			{
				SmallSet<LongRangePathNodeIndex> output;
				while(true)
				{
					output.insert(current);
					const auto& previous = closed[current];
					if(previous == LongRangePathNodeIndex::max())
						return output;
					current = previous;
				}
			}
			for(const LongRangePathNodeIndex& adjacentIndex : m_data[current].adjacent)
				if(!closed.contains(adjacentIndex))
				{
					closed.insert(adjacentIndex, current);
					open.push_back(adjacentIndex);
				}
		}
		// No path found.
		return {};
	}
	// TODO: path to itemQuery, path to fluid.
};
class AreaHasLongRangePathFinder
{
	SmallMap<MoveTypeId, LongRangePathFinderForMoveType> m_pathFinders;
	LongRangePathNodeIndex m_nextNodeIndex = LongRangePathNodeIndex::create(0);
public:
	void registerMoveType(const MoveTypeId& moveType);
	LongRangePathFinderForMoveType get(const MoveTypeId& moveType);
};