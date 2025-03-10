#include "blocks.h"
#include "../area/area.h"
#include "../materialType.h"
#include "plants.h"
#include "types.h"
void Blocks::plant_create(const BlockIndex& index, const PlantSpeciesId& plantSpecies, const Percent growthPercent)
{
	assert(m_plants[index].empty());
	m_plants[index] = m_area.getPlants().create({
		.location=index,
		.species=plantSpecies,
		.percentGrown=growthPercent,
	});
}
void Blocks::plant_set(const BlockIndex& index, const PlantIndex& plant)
{
	assert(m_plants[index].empty());
	m_plants[index] = plant;
}
void Blocks::plant_updateGrowingStatus(const BlockIndex& index)
{
	Plants& plants = m_area.getPlants();
	if(m_plants[index].exists())
		plants.updateGrowingStatus(m_plants[index]);
}
void Blocks::plant_setTemperature(const BlockIndex& index, const Temperature& temperature)
{
	Plants& plants = m_area.getPlants();
	if(m_plants[index].exists())
		plants.setTemperature(m_plants[index], temperature);
}
void Blocks::plant_erase(const BlockIndex& index)
{
	assert(m_plants[index].exists());
	m_plants[index].clear();
}
bool Blocks::plant_canGrowHereCurrently(const BlockIndex& index, const PlantSpeciesId& plantSpecies) const
{
	Temperature temperature = temperature_get(index);
	if(PlantSpecies::getMaximumGrowingTemperature(plantSpecies) < temperature || PlantSpecies::getMinimumGrowingTemperature(plantSpecies) > temperature)
		return false;
	if(PlantSpecies::getGrowsInSunLight(plantSpecies) != m_exposedToSky.check(index))
		return false;
	static MaterialTypeId dirtType = MaterialType::byName(L"dirt");
	BlockIndex below = getBlockBelow(index);
	if(below.exists() || !solid_is(below) || m_materialType[below] != dirtType)
		return false;
	return true;
}
bool Blocks::plant_canGrowHereAtSomePointToday(const BlockIndex& index, const PlantSpeciesId& plantSpecies) const
{
	Temperature temperature = temperature_getDailyAverageAmbient(index);
	if(PlantSpecies::getMaximumGrowingTemperature(plantSpecies) < temperature || PlantSpecies::getMinimumGrowingTemperature(plantSpecies) > temperature)
		return false;
	if(!plant_canGrowHereEver(index, plantSpecies))
		return false;
	return true;

}
bool Blocks::plant_canGrowHereEver(const BlockIndex& index, const PlantSpeciesId& plantSpecies) const
{
	if(PlantSpecies::getGrowsInSunLight(plantSpecies) != m_exposedToSky.check(index))
		return false;
	return plant_anythingCanGrowHereEver(index);
}
bool Blocks::plant_anythingCanGrowHereEver(const BlockIndex& index) const
{
	static MaterialTypeId dirtType = MaterialType::byName(L"dirt");
	BlockIndex below = getBlockBelow(index);
	if(below.empty() || !solid_is(below) || m_materialType[below] != dirtType)
		return false;
	return true;
}
PlantIndex Blocks::plant_get(const BlockIndex& index) const
{
	return m_plants[index];
}
bool Blocks::plant_exists(const BlockIndex& index) const
{
	return m_plants[index].exists();
}
