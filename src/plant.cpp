#include "plant.h"
#include "util.h"
#include "plant.h"
#include "block.h"
#include "area.h"
Plant::Plant(Block& l, const PlantSpecies& pt, uint32_t pg) : m_location(l), m_fluidSource(nullptr), m_plantSpecies(pt), m_percentGrown(pg), m_quantityToHarvest(0), m_percentFoliage(100), m_reservable(1), m_volumeDrinkRequested(0)
{
	assert(m_location.m_hasPlant.canGrowHere(m_plantSpecies));
	m_fluidEvent.schedule(m_plantSpecies.stepsNeedsFluidFrequency, *this);
	updateGrowingStatus();
}
void Plant::die()
{
	m_growthEvent.maybeUnschedule();
	m_fluidEvent.maybeUnschedule();
	m_temperatureEvent.maybeUnschedule();
	m_endOfHarvestEvent.maybeUnschedule();
	m_foliageGrowthEvent.maybeUnschedule();
	//TODO: Create rot away event.
}
void Plant::setTemperature(uint32_t temperature)
{
	if(temperature >= m_plantSpecies.minimumGrowingTemperature && temperature <= m_plantSpecies.maximumGrowingTemperature)
	{
		if(m_temperatureEvent.exists())
			m_temperatureEvent.unschedule();
	}
	else
		if(!m_temperatureEvent.exists())
			m_temperatureEvent.schedule(m_plantSpecies.stepsTillDieFromTemperature, *this);
	updateGrowingStatus();
}
void Plant::setHasFluidForNow()
{
	m_volumeDrinkRequested = 0;
	if(m_fluidEvent.exists())
		m_fluidEvent.unschedule();
	m_fluidEvent.schedule(m_plantSpecies.stepsNeedsFluidFrequency, *this);
	updateGrowingStatus();
	m_location.m_isPartOfFarmField.removeAllGiveFluidDesignations();
}
void Plant::setMaybeNeedsFluid()
{
	uint32_t stepsTillNextFluidEvent;
	if(hasFluidSource())
	{
		m_volumeDrinkRequested = 0;
		stepsTillNextFluidEvent = m_plantSpecies.stepsNeedsFluidFrequency;
	}
	else if(m_volumeDrinkRequested != 0)
	{
		die();
		return;
	}
	else // Needs fluid, stop growing and set death timer.
	{
		m_volumeDrinkRequested = util::scaleByPercent(m_plantSpecies.volumeFluidConsumed, getGrowthPercent());
		stepsTillNextFluidEvent = m_plantSpecies.stepsTillDieWithoutFluid;
		m_location.m_isPartOfFarmField.designateForGiveFluidIfPartOfFarmField(*this);
	}
	m_fluidEvent.schedule(stepsTillNextFluidEvent, *this);
	updateGrowingStatus();
}
void Plant::addFluid(uint32_t volume, const FluidType& fluidType)
{
	assert(fluidType == m_plantSpecies.fluidType);
	assert(volume <= m_volumeDrinkRequested);
	m_volumeDrinkRequested -= volume;
	if(m_volumeDrinkRequested == 0)
		setHasFluidForNow();
}
bool Plant::hasFluidSource()
{
	if(m_fluidSource != nullptr && m_fluidSource->m_fluids.contains(&m_plantSpecies.fluidType))
		return true;
	m_fluidSource = nullptr;
	for(Block* block : m_location.collectAdjacentsInRange(getRootRange()))
		if(block->m_fluids.contains(&m_plantSpecies.fluidType))
		{
			m_fluidSource = block;
			return true;
		}
	return false;
}
void Plant::setDayOfYear(uint32_t dayOfYear)
{
	if(!m_plantSpecies.annual && dayOfYear == m_plantSpecies.harvestData->dayOfYearToStart)
		setQuantityToHarvest();
}
void Plant::setQuantityToHarvest()
{
	uint32_t delay = m_plantSpecies.harvestData->daysDuration * Config::stepsPerDay;
	m_endOfHarvestEvent.schedule(delay, *this);
	m_quantityToHarvest = util::scaleByPercent(m_plantSpecies.harvestData->itemQuantity, getGrowthPercent());
	m_location.m_isPartOfFarmField.designateForHarvestIfPartOfFarmField(*this);
}
void Plant::harvest(uint32_t quantity)
{
	assert(quantity >= m_quantityToHarvest);
	m_quantityToHarvest -= quantity;
	if(m_quantityToHarvest == 0)
		endOfHarvest();
}
void Plant::endOfHarvest()
{
	m_location.m_isPartOfFarmField.removeAllHarvestDesignations();
	if(m_plantSpecies.annual)
		die();
	else
		m_quantityToHarvest = 0;
}
void Plant::updateGrowingStatus()
{
	if(m_volumeDrinkRequested == 0 && m_location.m_exposedToSky == m_plantSpecies.growsInSunLight && !m_temperatureEvent.exists() && getPercentFoliage() >= Config::minimumPercentFoliageForGrow)
	{
		if(!m_growthEvent.exists())
		{
			uint32_t delay = (m_percentGrown == 0 ?
					m_plantSpecies.stepsTillFullyGrown :
					((m_plantSpecies.stepsTillFullyGrown * m_percentGrown) / 100u));
			m_growthEvent.schedule(delay, *this);
		}
	}
	else
	{
		if(m_growthEvent.exists())
		{
			m_percentGrown = getGrowthPercent();
			m_growthEvent.unschedule();
		}
	}
}
uint32_t Plant::getGrowthPercent() const
{
	uint32_t output = m_percentGrown;
	if(m_growthEvent.exists())
	{
		if(m_percentGrown != 0)
			output += (m_growthEvent.percentComplete() * (100u - m_percentGrown)) / 100u;
		else
			output = m_growthEvent.percentComplete();
	}
	return output;
}
uint32_t Plant::getRootRange() const
{
	return m_plantSpecies.rootRangeMin + ((m_plantSpecies.rootRangeMax - m_plantSpecies.rootRangeMin) * getGrowthPercent()) / 100;
}
uint32_t Plant::getPercentFoliage() const
{
	uint32_t output = m_percentFoliage;
	if(m_foliageGrowthEvent.exists())
	{
		if(m_percentFoliage != 0)
			output += (m_growthEvent.percentComplete() * (100u - m_percentFoliage)) / 100u;
		else
			output = m_growthEvent.percentComplete();
	}
	return output;
}
uint32_t Plant::getFoliageMass() const
{
	uint32_t maxFoliageForType = util::scaleByPercent(m_plantSpecies.adultMass, Config::percentOfPlantMassWhichIsFoliage);
	uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, getGrowthPercent());
	return util::scaleByPercent(maxForGrowth, getPercentFoliage());
}
uint32_t Plant::getFluidDrinkVolume() const
{
	return util::scaleByPercent(m_plantSpecies.volumeFluidConsumed, Config::percentOfPlantMassWhichIsFoliage);
}
void Plant::removeFoliageMass(uint32_t mass)
{
	uint32_t maxFoliageForType = util::scaleByPercent(m_plantSpecies.adultMass, Config::percentOfPlantMassWhichIsFoliage);
	uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, getGrowthPercent());
	uint32_t percentRemoved = ((maxForGrowth - mass) / maxForGrowth) * 100u;
	m_percentFoliage = getPercentFoliage();
	assert(m_percentFoliage >= percentRemoved);
	m_percentFoliage -= percentRemoved;
	m_foliageGrowthEvent.maybeUnschedule();
	makeFoliageGrowthEvent();
	updateGrowingStatus();
}
void Plant::makeFoliageGrowthEvent()
{
	uint32_t delay = util::scaleByInversePercent(m_plantSpecies.stepsTillFoliageGrowsFromZero, m_percentFoliage);
	m_foliageGrowthEvent.schedule(delay, *this);
}
void Plant::foliageGrowth()
{
	m_percentFoliage = 100;
	updateGrowingStatus();
}
// HasPlant.
void HasPlant::addPlant(const PlantSpecies& plantSpecies, uint32_t growthPercent)
{
	assert(m_plant == nullptr);
	m_block.m_area->m_hasPlants.emplace(m_block, plantSpecies, growthPercent);
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
Plant& HasPlant::get()
{
	return *m_plant;
}
void HasPlant::setTemperature(uint32_t temperature)
{
	if(m_plant != nullptr)
		m_plant->setTemperature(temperature);
}
void HasPlants::emplace(Block& location, const PlantSpecies& species, uint32_t percentGrowth)
{
	m_plants.emplace_back(location, species, percentGrowth);
}
void HasPlants::erase(Plant& plant)
{
	std::erase_if(m_plants, [&](Plant& p) { return &p == &plant; });
}
