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
	m_world->m_seaLevel = paramaters.seaLevel;
	makeAttractors();
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
		m_attractorsWithMagnitudesAndDistanceExponentsAndMaxDistance.emplace_back(
			location,
			m_paramaters.oceanFloorAttractorWeight,
			m_paramaters.oceanFloorAttractorDistanceExponent,
			m_paramaters.oceanFloorMaxEffectRange
		);
		if(candidates.empty())
			break;
	}
	for(int i{0}; i < m_paramaters.mountainPeakAttractorCount; ++i)
	{
		Point3D location = m_random->getInCuboidSet(candidates);
		candidates.remove(location);
		location.setZ(m_paramaters.seaLevel + m_random->getInRange(m_paramaters.mountainPeakMinHeight.get(), m_paramaters.mountainPeakMaxHeight.get()));
		m_attractorsWithMagnitudesAndDistanceExponentsAndMaxDistance.emplace_back(
			location,
			m_paramaters.mountainPeakAttractorWeight,
			m_paramaters.mountainPeakAttractorDistanceExponent,
			m_paramaters.mountainMaxEffectRange
		);
		if(candidates.empty())
			break;
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
		Distance length{m_random->getInRange((DistanceWidth)1, Config::World::maxSizeOfBroadPhaseCuboid.get())};
		Point3D center{m_random->getInCuboidSet(toCover)};
		Distance highX = Distance::create(std::min((int)m_world->m_boundry.m_high.x().get(), center.x().get() + (width.get() / 2)));
		Distance highY = Distance::create(std::min((int)m_world->m_boundry.m_high.y().get(), center.y().get() + (length.get() / 2)));
		Distance lowX = Distance::create(std::max((int)m_world->m_boundry.m_low.x().get(), center.x().get() - (width.get() / 2)));
		Distance lowY = Distance::create(std::max((int)m_world->m_boundry.m_low.y().get(), center.y().get() - (length.get() / 2)));
		Cuboid cuboid{ Point3D(highX, highY, {0}), Point3D(lowX, lowY, {0}) };
		assert((cuboid.m_high.data >= cuboid.m_low.data).all());
		Distance zLevel = averageAttractors(center);
		cuboid.m_high.setZ(zLevel);
		assert(cuboid.m_low.z() == m_world->m_boundry.m_low.z());
		CuboidSet cuboidSet = CuboidSet::create(cuboid);
		m_world->m_solid.queryRemove(cuboidSet);
		m_world->m_solid.maybeInsert(cuboidSet, m_paramaters.primaryBedrockMaterial);
		cuboid.m_high.setZ(m_world->m_boundry.m_high.z());
		cuboid.m_low.setZ(m_world->m_boundry.m_high.z());
		CuboidSet newlyCovered = toCover.intersection(cuboid);
		toCover.remove(cuboid);
		areaCovered += newlyCovered.volume();
	}
	for(Cuboid cuboid : toCover)
	{
		Distance zLevel = averageAttractors(cuboid.getCenter());
		cuboid.m_high.setZ(zLevel);
		cuboid.m_low.setZ(m_world->m_boundry.m_low.z());
		m_world->m_solid.insert(cuboid, m_paramaters.primaryBedrockMaterial);
	}
	m_world->m_solid.prepare();
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
	int count = (float)candidates.volume() * m_paramaters.riverFractionToMakeSmallLakes;
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
	// Collect areas above solid.
	if(m_world->m_seaLevel > m_paramaters.treesMaxZLevel)
		return;
	Cuboid candidatesBoundry = m_world->m_boundry;
	candidatesBoundry.m_high.setZ(m_paramaters.treesMaxZLevel);
	candidatesBoundry.m_low.setZ(m_paramaters.seaLevel);
	candidatesBoundry.shift(Facing6::Below);
	CuboidSet candidates = m_world->m_solid.queryGetIntersection(candidatesBoundry);
	if(candidates.empty())
		return;
	for(Cuboid& cuboid : candidates)
		cuboid.m_low.setZ(cuboid.m_high.z());
	candidates.shift(Facing6::Above);
	m_world->m_solid.queryRemove(candidates);
	if(candidates.empty())
		return;
	CuboidSet nearWater;
	// Find near rivers.
	CuboidSet riversInflated = m_world->m_smallRivers.allCuboids();
	riversInflated.inflate(Config::World::maxHumidityEffectDistance);
	nearWater = candidates.intersection(riversInflated);
	// Find near coast.
	CuboidSet oceansInflated = m_world->m_fluid.queryGetAllCuboidsWithCondition(
		candidatesBoundry.inflated(Config::World::maxHumidityEffectDistance),
		[fluidType = m_world->m_oceanFluidType](FluidTypeId ft) { return ft == fluidType; }
	);
	oceansInflated.inflate(Config::World::maxHumidityEffectDistance);
	oceansInflated = oceansInflated.intersection(candidatesBoundry);
	nearWater.maybeAdd(oceansInflated.intersection(candidates));
	// Iterate point by point.
	for(Cuboid cuboid : nearWater)
		for(Point3D point : cuboid)
		{
			Percent humidity  = m_world->getHumidity(point);
			assert(humidity > 0);
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
	assert(!m_attractorsWithMagnitudesAndDistanceExponentsAndMaxDistance.empty());
	float weightAccumulator{0};
	float weightedAccumulator{0};
	for(auto [attractor, magnitude, distanceExponent, maxDistance] : m_attractorsWithMagnitudesAndDistanceExponentsAndMaxDistance)
	{
		float dx = point.x().get() - attractor.x().get();
		float dy = point.y().get() - attractor.y().get();
		float distance = std::sqrt(dx * dx + dy * dy);
		if(distance > maxDistance)
			continue;
		float weight;
		if(distance == 0.f)
			weight = magnitude;
		else
			weight = magnitude / std::pow(distance, distanceExponent);
		weightAccumulator += weight;
		weightedAccumulator += weight * attractor.z().get();
	}
	// sealevel attractor has no xy location, it affects all points equally.
	weightedAccumulator += m_world->m_seaLevel.get() * m_paramaters.seaLevelAttractorWeight;
	weightAccumulator += m_paramaters.seaLevelAttractorWeight;
	return Distance::create(std::round(weightedAccumulator / weightAccumulator));
}