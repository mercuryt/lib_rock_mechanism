#include "makeRivers.h"
#include "world.h"
#include "buildWorld.h"
#include "../dataStructures/smallMap.h"
// This shouldn't be needed. Why doesn't rtreeData.h work here?
#include "../dataStructures/rtreeData.hpp"
#include "../config/world.h"
#include "../rtreeHelpers/rtreeHelpers.h"
#include "../random.h"
#include "../space/adjacentOffsets.h"
MakeRivers::MakeRivers(BuildWorld& buildWorld) :
	m_world(*buildWorld.m_world),
	m_buildWorld(buildWorld)
{
	makeHeadwaters();
	makeRivers();
	recordConnections();
}
void MakeRivers::makeHeadwaters()
{
	Cuboid candidateLayer = m_world.m_boundry.slicedAtZ(m_buildWorld.m_paramaters.riverHeadwatersZLevel);
	CuboidSet candidates = candidateLayer.toSet();
	candidates.shift(Facing6::Below);
	candidates = m_world.m_solid.queryGetIntersection(candidates);
	if(candidates.empty())
		return;
	candidates.shift(Facing6::Above);
	m_world.m_solid.queryRemove(candidates);
	int headwatersCount = m_buildWorld.m_paramaters.riverHeadwatersCount;
	Random& random = *m_buildWorld.m_random;
	for(int i{0}; i < headwatersCount; ++i)
	{
		if(candidates.empty())
			return;
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
			SmallRiverData riverFlowingInto = m_world.m_smallRivers.queryGetOne(point);
			Point3D accumulationPoint;
			// If another river already crosses the headwaters point then don't make a path.
			if(riverFlowingInto.empty())
			{
				std::vector<Point3D> path = pathToRiverOrOcean(point);
				Distance distanceFromStart{0};
				int64_t flowRate = Config::World::riverDefaultFlowRate;
				for(Point3D step : path)
				{
					// Carve any canyons.
					m_world.m_solid.maybeRemove(step.inflatedDirection(Facing6::Above, Distance::max() / 2));
					SmallRiverData data{flowRate, m_world.m_oceanFluidType, distanceFromStart++, BitSet<uint8_t, 8u>()};
					m_world.m_smallRivers.insert(step, data);
				}
				// Find river to join.
				Point3D pathEnd = path.back();
				Cuboid adjacent = pathEnd.inflatedHorizontalAndBelow();
				auto [otherRiver, otherRiverCuboid] = m_world.m_smallRivers.queryGetOneWithCuboidAndCondition(
					adjacent,
					[this, pathEnd](Cuboid riverCuboid, SmallRiverData river){
						return riverCuboid.m_high.z() <= pathEnd.z() && river.fluidType == m_world.m_oceanFluidType;
					}
				);
				riverFlowingInto = otherRiver;
				if(otherRiverCuboid.exists())
					accumulationPoint = otherRiverCuboid.intersectionPoint(adjacent);
			}
			else
				accumulationPoint = point;
			if(!riverFlowingInto.empty())
			{
				// Follow river to end updating flowRate and distance.
				// If the point where the new river intersected the old one already had a longer distance from start then there will be no change to the distances updated.
				// This update is needed to preserve the relationship that distance always increases in the direction of flow.
				Distance distanceFromStart = std::max(distanceFromStart, riverFlowingInto.distanceFromStart);
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
						[distanceFromStart](SmallRiverData& river) {
							river.flowRate += Config::World::riverDefaultFlowRate;
							river.distanceFromStart = distanceFromStart;
						},
						[this](Cuboid, SmallRiverData river) { return river.fluidType == m_world.m_oceanFluidType; }
					);
					++distanceFromStart;
				}
			}
		}
}
void MakeRivers::recordConnections()
{
	for(Cuboid cuboid : m_world.m_smallRivers.getLeafCuboids())
		for(Point3D point : cuboid)
		{
			BitSet<uint8_t, 8> connections;
			for(int i{0}; i < 8; ++i)
			{
				auto [x, y] = adjacentOffsets::allWithSameZ[i];
				if(
					(x < 0 && point.x() == m_world.m_boundry.m_low.x()) ||
					(y < 0 && point.y() == m_world.m_boundry.m_low.y())
				)
						continue;
				Cuboid query = point.toCuboid();
				if(point.z() != m_world.m_boundry.m_low.z())
					query.inflateDirection(Facing6::Below);
				if(point.z() != m_world.m_boundry.m_high.z())
					query.inflateDirection(Facing6::Above);
				if(m_world.m_smallRivers.queryAny(query))
					connections.set(i);
			}
			m_world.m_smallRivers.updateActionOne(point, [connections](SmallRiverData& data) { data.connections = connections; });
		}
}
std::vector<Point3D> MakeRivers::pathToRiverOrOcean(Point3D headwaters) const
{
	// Doesn't flow tword other rivers but does stop when they are encountered.
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
		if(current.point.isTouchingFaceFromInside(m_world.m_boundry) || m_world.m_fluid.queryAny(adjacent) || m_world.m_smallRivers.queryAny(adjacent))
		{
			// End found.
			std::vector<Point3D> route;
			Point3D point = current.point;
			while(point != headwaters)
			{
				route.push_back(point);
				point = toFrom[point];
			}
			route.push_back(headwaters);
			std::ranges::reverse(route);
			return route;
		}
		// Find costs for adjacent and add to open list.
		if(current.point.z() > m_world.m_boundry.m_low.z())
		{
			// Next level down.
			CuboidSet candidates = current.point.below().inflatedHorizontal().intersection(m_world.m_boundry).toSet();
			// Don't go straight down.
			candidates.remove(current.point.below());
			candidates.maybeRemove(closedList);
			if(!candidates.empty())
			{
				m_world.m_solid.queryRemove(candidates);
				if(!candidates.empty())
				{
					closedList.add(candidates);
					for(Cuboid cuboid : candidates)
						for(Point3D candidate : cuboid)
						{
							// No cost when going down.
							openList.emplace_back(candidate, current.cost, priority(candidate, current.cost));
							toFrom.insert(candidate, current.point);
						}
				}
			}
		}
		// Current Level.
		CuboidSet candidates = current.point.inflatedHorizontal().intersection(m_world.m_boundry).toSet();
		candidates.maybeRemove(closedList);
		if(candidates.empty())
			continue;
		closedList.add(candidates);
		for(Cuboid cuboid : candidates)
			for(Point3D candidate : cuboid)
			{
				// Horizontal movement costs 1.
				int cost = current.cost + 1;
				if(m_world.m_solid.queryAny(candidate))
					// Cutting through solid costs extra per zlevel.
					cost += RTreeHelpers::getContiguousDistanceInDirection(m_world.m_solid, candidate.toCuboid(), Facing6::Above).get() * Config::World::riverCanyonCost;
				openList.emplace_back(candidate, cost, priority(candidate, cost));
				toFrom.insert(candidate, current.point);
			}
	}
	// no path found.
	return {};
}