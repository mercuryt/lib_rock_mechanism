#include "makeRivers.h"
#include "world.h"
#include "buildWorld.h"
#include "../dataStructures/smallMap.h"
// This shouldn't be needed. Why doesn't rtreeData.h work here?
#include "../dataStructures/rtreeData.hpp"
#include "../config/world.h"
#include "../rtreeHelpers/rtreeHelpers.h"
#include "../random.h"
MakeRivers::MakeRivers(BuildWorld& buildWorld) :
	m_world(*buildWorld.m_world),
	m_buildWorld(buildWorld)
{
	makeHeadwaters();
	makeRivers();
}
void MakeRivers::makeHeadwaters()
{
	Cuboid candidateLayer = m_world.m_boundry.slicedAtZ(m_buildWorld.m_paramaters.riverHeadwatersZLevel);
	CuboidSet candidates = candidateLayer.toSet();
	candidates.shift(Facing6::Below);
	candidates = m_world.m_solid.queryGetIntersection(candidates);
	candidates.shift(Facing6::Above);
	m_world.m_solid.queryRemove(candidates);
	int headwatersCount = m_buildWorld.m_paramaters.riverHeadwatersCount;
	Random& random = *m_buildWorld.m_random;
	for(int i{0}; i < headwatersCount; ++i)
	{
		Point3D point = random.getInCuboidSet(candidates);
		candidates.remove(point);
		m_headwaters.add(point);
	}
}
void MakeRivers::makeRivers()
{
	for(Cuboid cuboid : m_headwaters)
		for(Point3D point : cuboid)
		{
			std::vector<Point3D> path = pathToRiverOrOcean(point);
			int distanceFromStart{0};
			int64_t flowRate = Config::World::riverDefaultFlowRate;
			for(Point3D step : path)
			{
				// Carve any canyons.
				m_world.m_solid.remove(step.inflatedDirection(Facing6::Above, Distance::max() / 2));
				SmallRiverData data{flowRate, m_world.m_oceanFluidType, distanceFromStart++};
				m_world.m_smallRivers.insert(step, data);
			}
			// Add flow rates for rivers that join together.
			//  Find river to join.
			Point3D pathEnd = path.back();
			Cuboid adjacent = pathEnd.inflatedHorizontalAndBelow();
			auto [otherRiver, otherRiverCuboid] = m_world.m_smallRivers.queryGetOneWithCuboidAndCondition(
				adjacent,
				[this, pathEnd](Cuboid riverCuboid, SmallRiverData river){
					return riverCuboid.m_high.z() <= pathEnd.z() && river.fluidType == m_world.m_oceanFluidType;
				}
			);
			if(!otherRiver.empty())
			{
				// Follow river to end updating flowRate and distance.
				// If the point where the new river intersected the old one already had a longer distance from start then there will be no change to the distances updated.
				// This update is needed to preserve the relationship that distance always increases in the direction of flow.
				Point3D accumulationPoint = otherRiverCuboid.intersectionPoint(adjacent);
				distanceFromStart = std::max(distanceFromStart, otherRiver.distanceFromStart);
				while(true)
				{
					Cuboid adjacentToAccumulationPoint = accumulationPoint.inflatedHorizontalAndBelow();
					Cuboid candidates = m_world.m_smallRivers.queryGetOneCuboidWithCondition(adjacentToAccumulationPoint, [this, distanceFromStart](SmallRiverData river){
						return river.distanceFromStart > distanceFromStart && river.fluidType == m_world.m_oceanFluidType;
					});
					if(candidates.empty())
						break;
					accumulationPoint = candidates.intersectionPoint(adjacentToAccumulationPoint);
					m_world.m_smallRivers.updateActionWithConditionOne(
						accumulationPoint,
						[flowRate, distanceFromStart](SmallRiverData& river) {
							river.flowRate += flowRate;
							river.distanceFromStart = distanceFromStart;
						},
						[this](Cuboid, SmallRiverData river) { return river.fluidType == m_world.m_oceanFluidType; }
					);
					++distanceFromStart;
				}
			}
		}
}
std::vector<Point3D> MakeRivers::pathToRiverOrOcean(Point3D headwaters) const
{
	struct Open{
		Point3D point;
		int cost;
		int priority;
	};
	std::vector<Open> openList;
	CuboidSet closedList;
	SmallMap<Point3D, Point3D> toFrom;
	auto priority = [this](const Point3D point, int cost) -> int {
		Distance distanceToFluid = m_world.m_fluid.distanceWithCondition(
			point,
			Config::World::maxRangeOfRiverToOcean,
			// Fluid river is flowing to meet must be at the same or lower elevation.
			[point](Cuboid cuboid, FluidTypeId){ return cuboid.m_high.z() <= point.z(); }
		);
		return distanceToFluid.get() + cost;
	};
	openList.emplace_back(headwaters, 0, priority(headwaters, 0));
	closedList.add(headwaters);
	while(!openList.empty())
	{
		auto iter = std::ranges::min_element(openList, {}, &Open::priority);
		Open current = *iter;
		(*iter) = openList.back();
		openList.pop_back();
		Cuboid adjacent = current.point.inflated();
		if(m_world.m_fluid.queryAny(adjacent))
		{
			// End found.
			std::vector<Point3D> route;
			Point3D point = current.point;
			while(point != headwaters)
			{
				route.push_back(point);
				point = toFrom[point];
			}
			return route;
		}
		CuboidSet adjacentSet = adjacent.toSet();
		m_world.m_solid.queryRemove(adjacentSet);
		Cuboid adjacentBoundry = adjacentSet.boundry();
		int cost = current.cost;
		if(adjacentBoundry.m_low.z() > current.point.z())
		{
			// Nothing on the next level down or this level.
			adjacentSet = adjacent.inflatedHorizontal().toSet();
			adjacentSet.remove(closedList);
			for(Cuboid cuboid : adjacentSet)
				for(Point3D point : cuboid)
				{
					Distance verticalHeightToCarveThrough = RTreeHelpers::getContiguousDistanceInDirection(m_world.m_solid, point.toCuboid(), Facing6::Above);
					int pointCost = cost + (Config::World::riverCanyonCost * verticalHeightToCarveThrough.get());
					openList.emplace_back(point, pointCost, priority(point, cost));
					toFrom.insert(point, current.point);
				}
		}
		else
		{
			// Moving on the same level cost one, moving down is free.
			if(adjacentBoundry.m_low.z() == current.point.z())
				++cost;
			CuboidSet lowAdjacent = adjacentSet.slicedAtZ(adjacentBoundry.m_low.z());
			closedList.add(lowAdjacent);
			for(Cuboid cuboid : lowAdjacent)
				for(Point3D point : cuboid)
				{
					openList.emplace_back(point, cost, priority(point, cost));
					toFrom.insert(point, current.point);
				}
		}
	}
	// no path found.
	return {};
}