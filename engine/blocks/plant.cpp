#include "blocks.h"
#include "../area.h"
#include "../materialType.h"
#include "../plant.h"
#include "types.h"
void Blocks::plant_create(BlockIndex index, const PlantSpecies& plantSpecies, Percent growthPercent)
{
	assert(m_plants.at(index) == nullptr);
	m_plants.at(index) = &m_area.m_hasPlants.emplace(index, plantSpecies, nullptr, growthPercent);
}
void Blocks::plant_set(BlockIndex index, Plant& plant)
{
	m_plants.at(index) = &plant;
}
void Blocks::plant_updateGrowingStatus(BlockIndex index)
{
	if(m_plants.at(index) != nullptr)
		m_plants.at(index)->updateGrowingStatus();
}
void Blocks::plant_clearPointer(BlockIndex index)
{
	assert(m_plants.at(index) != nullptr);
	m_plants.at(index) = nullptr;
}
void Blocks::plant_setTemperature(BlockIndex index, Temperature temperature)
{
	if(m_plants.at(index) != nullptr)
		m_plants.at(index)->setTemperature(temperature);
}
void Blocks::plant_erase(BlockIndex index)
{
	assert(m_plants.at(index) != nullptr);
	m_plants.at(index) = nullptr;
}
bool Blocks::plant_canGrowHereCurrently(BlockIndex index, const PlantSpecies& plantSpecies) const
{
	Temperature temperature = temperature_get(index);
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(plantSpecies.growsInSunLight != m_outdoors[index])
		return false;
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	BlockIndex below = getBlockBelow(index);
	if(below != BLOCK_INDEX_MAX || !solid_is(below) || m_materialType.at(below) != &dirtType)
		return false;
	return true;
}
bool Blocks::plant_canGrowHereAtSomePointToday(BlockIndex index, const PlantSpecies& plantSpecies) const
{
	Temperature temperature = temperature_getDailyAverageAmbient(index);
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(!plant_canGrowHereEver(index, plantSpecies))
		return false;
	return true;

}
bool Blocks::plant_canGrowHereEver(BlockIndex index, const PlantSpecies& plantSpecies) const
{
	if(plantSpecies.growsInSunLight != m_outdoors[index])
		return false;
	return plant_anythingCanGrowHereEver(index);
}
bool Blocks::plant_anythingCanGrowHereEver(BlockIndex index) const
{
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	BlockIndex below = getBlockBelow(index);
	if(below == BLOCK_INDEX_MAX || !solid_is(below) || m_materialType.at(below) != &dirtType)
		return false;
	return true;
}
Plant& Blocks::plant_get(BlockIndex index)
{
	return *m_plants.at(index);
}
bool Blocks::plant_exists(BlockIndex index) const
{
	return m_plants.at(index);
}
