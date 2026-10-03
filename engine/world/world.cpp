#include "world.h"
#include "buildArea.h"
#include "../simulation/simulation.h"
#include "../simulation/hasAreas.h"
#include "../config/world.h"
#include <numeric>
World::World(Cuboid boundry) :
	m_boundry(boundry),
	m_oceanFluidType(FluidType::byName("water"))
{ }
AreaId World::createArea(Simulation& simulation, Point3D location, bool proceduralGeneration)
{
	Area* area;
	if(!proceduralGeneration)
	{
		Distance size = Config::World::defaultAreaSize;
		area = &simulation.m_hasAreas->createArea(size, size, size, true);
	}
	else
	{
		BuildArea buildArea(*this, simulation, location);
		area = buildArea.m_area;
	}
	area->m_location = location;
	m_areas.insert(location, area->m_id);
	return area->m_id;
}
Percent World::getHumidity(Point3D location) const
{
	Cuboid range = location.inflated(Config::World::maxHumidityEffectDistance);
	int volumeContainsFluidSource = m_fluid.queryIntersectionVolume(range) + m_smallRivers.queryIntersectionVolume(range);
	return Percent::create(volumeContainsFluidSource) * Config::World::humidityPercentPerFluidBlock;
}
MaterialTypeId World::getBedrockType(Point3D location) const
{
	MaterialTypeId output = m_solid.queryGetOne(location);
	if(output.empty())
		output = m_solid.queryGetOne(location.below());
	return output;
}
Temperature World::getAverageTemperature(Point3D location) const
{
	Distance equator = m_boundry.sizeY() / 2;
	Temperature maxAmbiant = m_maxAmbiantTemperature;
	Temperature minAmbiant = m_minAmbiantTemperature;
	// Distance from equator reduces base temperature.
	Temperature output{(TemperatureWidth)util::scaleByFractionRange(minAmbiant.get(), maxAmbiant.get(), location.y().get(), equator.get())};
	// Altititude reduces base temperature.
	if(location.z() >= m_seaLevel)
	{
		float altitudeAdjustment = 1.f - (Config::World::temperatureLossFractionPerUnitAltitude * (float)(location.z() - m_seaLevel).get());
		output *= altitudeAdjustment;
	}
	return output;
}
int World::getFoliageMass(Point3D location) const
{
	// Derive from temperature and humidity.
	Temperature averageTemperature = getAverageTemperature(location);
	Temperature differenceBetweenAverageAndIdeal{(TemperatureWidth)std::abs(averageTemperature.get() - Config::World::idealTemperatureForPlants.get())};
	float temperatureModifier = differenceBetweenAverageAndIdeal.get() * Config::World::fractionOfFoliageMassToLosePerPointTemperatureFromIdeal;
	Percent humidity = getHumidity(location);
	return util::scaleByPercent(Config::World::maxFoliageMassForArea * temperatureModifier, humidity);
}
Area& World::getOrCreateArea(Simulation& simulation, Point3D location)
{
	AreaId id = m_areas.queryGetOne(location);
	if(id.empty())
		id = createArea(simulation, location);
	return simulation.m_hasAreas->getById(id);

}