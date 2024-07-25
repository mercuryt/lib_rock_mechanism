#include "blocks.h"
#include "../area.h"
#include "../materialType.h"
#include "plants.h"
#include "types.h"
void Blocks::plant_create(BlockIndex index, const PlantSpecies& plantSpecies, Percent growthPercent)
{
	assert(m_plants.at(index).empty());
	m_plants.at(index) = m_area.getPlants().create({
		.location=index, 
		.species=plantSpecies, 
		.percentGrown=growthPercent,
	});
}
void Blocks::plant_set(BlockIndex index, PlantIndex plant)
{
	m_plants.at(index) = plant;
}
void Blocks::plant_updateGrowingStatus(BlockIndex index)
{
	Plants& plants = m_area.getPlants();
	if(m_plants.at(index).exists())
		plants.updateGrowingStatus(index);
}
void Blocks::plant_clearPointer(BlockIndex index)
{
	assert(m_plants.at(index).exists());
	m_plants.at(index).clear();
}
void Blocks::plant_setTemperature(BlockIndex index, Temperature temperature)
{
	Plants& plants = m_area.getPlants();
	if(m_plants.at(index).exists())
		plants.setTemperature(index, temperature);
}
void Blocks::plant_erase(BlockIndex index)
{
	assert(m_plants.at(index).exists());
	m_plants.at(index).clear();
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
	if(below.exists() || !solid_is(below) || m_materialType.at(below) != &dirtType)
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
	if(below.empty() || !solid_is(below) || m_materialType.at(below) != &dirtType)
		return false;
	return true;
}
PlantIndex Blocks::plant_get(BlockIndex index)
{
	return m_plants.at(index);
}
bool Blocks::plant_exists(BlockIndex index) const
{
	return m_plants.at(index);
}
