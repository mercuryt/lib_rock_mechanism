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
AreaId World::createArea(Simulation& simulation, Point3D location)
{
	BuildArea buildArea(*this, simulation, location);
	return buildArea.m_area->m_id;
}
Percent World::getHumidity(Point3D location) const
{
	Percent output{0};
	CuboidSet edge = location.toSet();
	for(Distance range{0}; range <= Config::World::maxHumidityEffectDistance; ++range)
	{
		CuboidSet copy = edge;
		edge.inflate({1});
		edge.removeAll(copy);
		int volume = edge.volume();
		int volumeContainsFluidSource = m_fluid.queryCount(edge) + m_smallRivers.queryIntersectionVolume(edge) + m_smallLakes.queryIntersectionVolume(edge);
		Percent percentagePerBlock{volume / 100};
		output += percentagePerBlock * volumeContainsFluidSource;
		if(output >= 100)
			return {100};
	}
	return output;
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