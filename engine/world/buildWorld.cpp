#include "buildWorld.h"
#include "world.h"
#include "makeRivers.h"
#include "../simulation/simulation.h"
#include "../random.h"
#include "../config/world.h"
BuildWorld::BuildWorld(Simulation& simulation, WorldParamaters& paramaters) :
	m_paramaters(paramaters),
	m_random(&simulation.m_random)
{
	m_world = std::make_unique<World>(paramaters.boundry);
	makeLand();
	makeOceans();
	makeRivers();
	makeLakes();
	makeTrees();
}
void BuildWorld::makeAttractors()
{
	Cuboid above = m_world->m_boundry.getFaceAbove();
	CuboidSet candidates = above.toSet();
	for(int i{0}; i < m_paramaters.oceanFloorAttractorCount; ++i)
	{
		Point3D location = m_random->getInCuboidSet(candidates);
		candidates.remove(location.inflatedHorizontal(Config::World::minDistanceBetweenWorldAttractors));
		location.setZ(m_paramaters.seaLevel - m_random->getInRange(m_paramaters.oceanFloorMinDepth.get(), m_paramaters.oceanFloorMaxDepth.get()));
		m_attractorsWithMagnitudesAndDistanceExponents.emplace_back(
			location,
			m_paramaters.oceanFloorAttractorWeight,
			m_paramaters.oceanFloorAttractorDistanceExponent
		);
	}
	for(int i{0}; i < m_paramaters.mountainPeakAttractorCount; ++i)
	{
		Point3D location = m_random->getInCuboidSet(candidates);
		candidates.remove(location);
		location.setZ(m_paramaters.seaLevel + m_random->getInRange(m_paramaters.mountainPeakMinHeight.get(), m_paramaters.mountainPeakMaxHeight.get()));
		m_attractorsWithMagnitudesAndDistanceExponents.emplace_back(
			location,
			m_paramaters.mountainPeakAttractorWeight,
			m_paramaters.mountainPeakAttractorDistanceExponent
		);
	}
}
void BuildWorld::makeLand()
{
	Cuboid above = m_world->m_boundry.getFaceAbove();
	int64_t areaToCover = above.volume();
	int64_t areaCovered{0};
	int64_t areaToCoverBroadPhase = areaToCover * Config::World::areaToCoverBroadPhaseWorldGen;
	CuboidSet toCover{above.toSet()};
	while(areaCovered < areaToCoverBroadPhase)
	{
		Distance width{m_random->getInRange((DistanceWidth)1, Config::World::maxSizeOfBroadPhaseCuboid.get())};
		Distance height{m_random->getInRange((DistanceWidth)1, Config::World::maxSizeOfBroadPhaseCuboid.get())};
		Point3D center{m_random->getInCuboidSet(toCover)};
		Cuboid cuboid{
			Point3D(center.x() + (width - 1 / 2), center.y() + (height - 1 / 2), Distance::create(0)),
			Point3D(center.x() - (width - 1 / 2), center.y() - (height - 1 / 2), Distance::create(0))
		};
		Distance zLevel = averageAttractors(center);
		cuboid.m_high.setZ(zLevel);
		m_world->m_solid.insert(cuboid, m_paramaters.primaryBedrockMaterial);
		CuboidSet newlyCovered = toCover.intersection(cuboid);
		toCover.remove(cuboid);
		areaCovered += newlyCovered.volume();
	}
	if(!toCover.empty())
		for(Cuboid cuboid : toCover)
		{
			Distance zLevel = averageAttractors(cuboid.getCenter());
			cuboid.m_high.setZ(zLevel);
			m_world->m_solid.insert(cuboid, m_paramaters.primaryBedrockMaterial);
		}
}
void BuildWorld::makeOceans()
{
	Cuboid toFillCuboid = m_world->m_boundry;
	toFillCuboid.m_high.setZ(m_world->m_seaLevel.subtractWithMinimum(1));
	CuboidSet toFill = toFillCuboid.toSet();
	m_world->m_solid.queryRemove(toFill);
	m_world->m_ocean.insert(toFill);
	m_world->m_fluid.insert(toFill, m_paramaters.primaryFluid);
}
void BuildWorld::makeRivers()
{
	MakeRivers(*this);
}
void BuildWorld::makeLakes()
{
	Random& random = *m_random;
	CuboidSet candidates = m_world->m_smallRivers.allCuboids();
	int count = candidates.volume() * m_paramaters.riverFractionToMakeSmallLakes;
	while(count != 0)
	{
		Point3D lake = random.getInCuboidSet(candidates);
		candidates.remove(lake);
		m_world->m_smallLakes.insert(lake);
		--count;
	}
}
void BuildWorld::makeTrees()
{
	CuboidSet candidates = m_world->m_boundry.slicedAtZ(m_world->m_seaLevel).toSet();
	m_world->m_fluid.queryRemove(candidates);
	for(Cuboid cuboid : candidates)
		for(Point3D point : cuboid)
		{
			Percent humidity  = m_world->getHumidity(point);
			if(humidity < Config::World::minimumHumidityForTrees)
				continue;
			int trees = m_random->getInRange(
				Config::World::minimumTreesPerPercentHumidity * humidity.get(),
				Config::World::maximumTreesPerPercentHumidity * humidity.get()
			);
			float deltaFromIdealTemperature = m_world->getAverageTemperature(point).get() - Config::World::idealTemperatureForPlants.get();
			float reduction = deltaFromIdealTemperature / Config::World::treesMaxDeltaFromIdealTemperature.get();
			trees = trees * reduction;
			m_world->m_trees.insert(point, {trees});
		}
}
Distance BuildWorld::averageAttractors(Point3D point)
{
	assert(!m_attractorsWithMagnitudesAndDistanceExponents.empty());
	float weightAccumulator{0};
	float weightedAccumulator{0};
	for(auto [attractor, magnitude, distanceExponent] : m_attractorsWithMagnitudesAndDistanceExponents)
	{
		float dx = point.x().get() - attractor.x().get();
		float dy = point.y().get() - attractor.y().get();
		float distance = std::sqrt(dx * dx + dy * dy);
		if(distance == 0.f)
			return attractor.z();
		distance = std::pow(distance, distanceExponent);
		float weight = magnitude / distance;
		weightAccumulator += weight;
		weightedAccumulator += weight * attractor.z().get();
	}
	// sealevel attractor has no xy location, it affects all points equally.
	weightAccumulator += m_world->m_seaLevel.get() * Config::World::seaLevelAttractorWeight;
	weightAccumulator += Config::World::seaLevelAttractorWeight;
	return {DistanceWidth(weightedAccumulator / weightAccumulator)};
}