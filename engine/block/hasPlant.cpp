#include "hasPlant.h"
#include "../block.h"
#include "../area.h"
#include "../materialType.h"
#include "../plant.h"
void BlockHasPlant::createPlant(const PlantSpecies& plantSpecies, Percent growthPercent)
{
	assert(m_plant == nullptr);
	m_plant = &m_block.m_area->m_hasPlants.emplace(m_block, plantSpecies, nullptr, growthPercent);
}
void BlockHasPlant::updateGrowingStatus()
{
	if(m_plant != nullptr)
		m_plant->updateGrowingStatus();
}
void BlockHasPlant::clearPointer()
{
	assert(m_plant != nullptr);
	m_plant = nullptr;
}
void BlockHasPlant::setTemperature(Temperature temperature)
{
	if(m_plant != nullptr)
		m_plant->setTemperature(temperature);
}
void BlockHasPlant::erase()
{
	assert(m_plant != nullptr);
	m_plant = nullptr;
}
bool BlockHasPlant::canGrowHereCurrently(const PlantSpecies& plantSpecies) const
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
bool BlockHasPlant::canGrowHereAtSomePointToday(const PlantSpecies& plantSpecies) const
{
	Temperature temperature = m_block.m_blockHasTemperature.getDailyAverageAmbientTemperature();
	if(plantSpecies.maximumGrowingTemperature < temperature || plantSpecies.minimumGrowingTemperature > temperature)
		return false;
	if(!canGrowHereEver(plantSpecies))
		return false;
	return true;

}
bool BlockHasPlant::canGrowHereEver(const PlantSpecies& plantSpecies) const
{
	if(plantSpecies.growsInSunLight != m_block.m_outdoors)
		return false;
	return anythingCanGrowHereEver();
}
bool BlockHasPlant::anythingCanGrowHereEver() const
{
	static const MaterialType& dirtType = MaterialType::byName("dirt");
	if(m_block.m_adjacents[0] == nullptr || !m_block.m_adjacents[0]->isSolid() || m_block.m_adjacents[0]->getSolidMaterial() != dirtType)
		return false;
	return true;
}
