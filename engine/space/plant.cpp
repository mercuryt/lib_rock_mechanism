#include "space.h"
#include "../area/area.h"
#include "../definitions/materialType.h"
#include "plants.h"
#include "numericTypes/types.h"
PlantIndex Space::plant_create(const Point3D& point, const PlantSpeciesId& plantSpecies, const Percent growthPercent)
{
	assert(!m_plants.queryAny(point));
	return m_area.getPlants().create({
		.location=point,
		.species=plantSpecies,
		.percentGrown=growthPercent,
	});
}
void Space::plant_set(const Point3D& point, const PlantIndex& plant)
{
	assert(!m_plants.queryAny(point));
	m_plants.maybeInsert(point, plant);
}
void Space::plant_updateGrowingStatus(const Point3D& point)
{
	Plants& plants = m_area.getPlants();
	const PlantIndex& index = m_plants.queryGetOne(point);
	if(index.exists())
		plants.updateGrowingStatus(index);
}
void Space::plant_setTemperature(const Point3D& point, const Temperature& temperature)
{
	Plants& plants = m_area.getPlants();
	const PlantIndex& index = m_plants.queryGetOne(point);
	if(index.exists())
		plants.setTemperature(index, temperature);
}
void Space::plant_erase(const Point3D& point)
{
	assert(m_plants.queryAny(point));
	m_plants.maybeRemove(point);
}
bool Space::plant_canGrowHereCurrently(const Point3D& point, const PlantSpeciesId& plantSpecies) const
{
	Temperature temperature = temperature_get(point);
	if(PlantSpecies::getMaximumGrowingTemperature(plantSpecies) < temperature || PlantSpecies::getMinimumGrowingTemperature(plantSpecies) > temperature)
		return false;
	if(PlantSpecies::getGrowsInSunLight(plantSpecies) != m_exposedToSky.check(point))
		return false;
	static MaterialTypeId dirtType = MaterialType::byName("dirt");
	Point3D below = point.below();
	if(below.exists() || !solid_is(below) || m_solid.queryGetOne(below) != dirtType)
		return false;
	return true;
}
bool Space::plant_canGrowHereAtSomePointToday(const Point3D& point, const PlantSpeciesId& plantSpecies) const
{
	Temperature temperature = temperature_getDailyAverageAmbient(point);
	if(PlantSpecies::getMaximumGrowingTemperature(plantSpecies) < temperature || PlantSpecies::getMinimumGrowingTemperature(plantSpecies) > temperature)
		return false;
	if(!plant_canGrowHereEver(point, plantSpecies))
		return false;
	return true;

}
bool Space::plant_canGrowHereEver(const Point3D& point, const PlantSpeciesId& plantSpecies) const
{
	if(PlantSpecies::getGrowsInSunLight(plantSpecies) != m_exposedToSky.check(point))
		return false;
	return plant_anythingCanGrowHereEver(point);
}
bool Space::plant_anythingCanGrowHereEver(const Point3D& point) const
{
	static MaterialTypeId dirtType = MaterialType::byName("dirt");
	Point3D below = point.below();
	if(below.empty() || !solid_is(below) || m_solid.queryGetOne(below) != dirtType)
		return false;
	return true;
}
PlantIndex Space::plant_get(const Point3D& point) const
{
	return m_plants.queryGetOne(point);
}
bool Space::plant_exists(const Point3D& point) const
{
	return m_plants.queryAny(point);
}
