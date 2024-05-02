#include "hasPlant.h"
#include "../block.h"
#include "../area.h"
#include "../materialType.h"
#include "../plant.h"
// HasPlant.
void HasPlant::createPlant(const PlantSpecies& plantSpecies, Percent growthPercent)
{
	assert(m_plant == nullptr);
	m_plant = &m_block.m_area->m_hasPlants.emplace(m_block, plantSpecies, nullptr, growthPercent);
}
void HasPlant::updateGrowingStatus()
{
	if(m_plant != nullptr)
		m_plant->updateGrowingStatus();
}
void HasPlant::clearPointer()
{
	assert(m_plant != nullptr);
	m_plant = nullptr;
}
void HasPlant::setTemperature(Temperature temperature)
{
	if(m_plant != nullptr)
		m_plant->setTemperature(temperature);
}
void HasPlant::erase()
{
	assert(m_plant != nullptr);
	m_plant = nullptr;
}
bool HasPlant::canGrowHereCurrently(const PlantSpecies& plantSpecies) const
{
	Temperature temperature = m_block.m_blockHasTemperature.get();
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(plantSpecies.growsInSunLight != m_block.m_outdoors)
		return false;
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	if(m_block.m_adjacents[0] == nullptr || !m_block.m_adjacents[0]->isSolid() || m_block.m_adjacents[0]->getSolidMaterial() != dirtType)
		return false;
	return true;
}
bool HasPlant::canGrowHereAtSomePointToday(const PlantSpecies& plantSpecies) const
{
	Temperature temperature = m_block.m_blockHasTemperature.getDailyAverageAmbientTemperature();
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(!canGrowHereEver(plantSpecies))
		return false;
	return true;

}
bool HasPlant::canGrowHereEver(const PlantSpecies& plantSpecies) const
{
	if(plantSpecies.growsInSunLight != m_block.m_outdoors)
		return false;
	return anythingCanGrowHereEver();
}
bool HasPlant::anythingCanGrowHereEver() const
{
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	if(m_block.m_adjacents[0] == nullptr || !m_block.m_adjacents[0]->isSolid() || m_block.m_adjacents[0]->getSolidMaterial() != dirtType)
		return false;
	return true;
}
