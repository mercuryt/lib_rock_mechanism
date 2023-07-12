#include "plant.h"
Plant::Plant(Block& l, const PlantSpecies& pt, uint32_t pg = 0) : m_location(l), m_fluidSource(nullptr), m_plantSpecies(pt), m_percentGrown(pg), m_quantityToHarvest(0), m_hasFluid(true), m_percentFoliage(100), m_reservable(1)
{
	assert(m_location.plantTypeCanGrow(m_plantSpecies));
	m_location.m_plants.push_back(this);
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
void Plant::applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature)
{
	(void)oldTemperature;
	if(newTemperature >= m_plantSpecies.minimumGrowingTemperature && newTemperature <= m_plantSpecies.maximumGrowingTemperature)
	{
		if(!m_temperatureEvent.empty())
			m_temperatureEvent->unschedule();
	}
	else
		if(m_temperatureEvent.empty())
			m_temperatureEvent.schedule(m_plantSpecies.stepsTillDieFromTemperature, *this);
	updateGrowingStatus();
}
void Plant::setHasFluidForNow()
{
	m_hasFluid = true;
	if(!m_fluidEvent.empty())
		m_fluidEvent->unschedule();
	m_fluidEvent.schedule(m_plantSpecies.stepsNeedsFluidFrequency, *this);
	updateGrowingStatus();
	m_location.m_isPartOfFarmField.removeAllGiveFluidDesignations();
}
void Plant::setMaybeNeedsFluid()
{
	uint32_t stepsTillNextFluidEvent;
	if(hasFluidSource())
	{
		m_hasFluid = true;
		stepsTillNextFluidEvent = m_plantSpecies.stepsNeedsFluidFrequency;
	}
	else if(!m_hasFluid)
	{
		die();
		return;
	}
	else // Needs fluid, stop growing and set death timer.
	{
		m_hasFluid = false;
		stepsTillNextFluidEvent = m_plantSpecies.stepsTillDieWithoutFluid;
		m_location.m_isPartOfFarmField.designateForGiveFluidIfPartOfFarmField;
	}
	m_fluidEvent.schedule(stepsTillNextFluidEvent, *this);
	updateGrowingStatus();
}
bool Plant::hasFluidSource() const
{
	if(m_fluidSource != nullptr && m_fluidSource->m_fluids.contains(&m_plantSpecies.fluidType))
		return true;
	m_fluidSource = nullptr;
	for(Block* block : util<Block>::collectAdjacentsInRange(getRootRange(), m_location))
		if(block->m_fluids.contains(&m_plantSpecies.fluidType))
		{
			m_fluidSource = block;
			return true;
		}
	return false;
}
void Plant::setDayOfYear(uint32_t dayOfYear)
{
	if(!m_plantSpecies.annual && dayOfYear == m_plantSpecies.dayOfYearForHarvest)
		setQuantityToHarvest();
}
void Plant::setQuantityToHarvest()
{
	m_endOfHarvestEvent.schedule(m_plantSpecies.stepsTillEndOfHarvest, *this);
	m_quantityToHarvest = util::scaleByPercent(m_plantSpecies.fruitQuantityWhenFullyGrown, getGrowthPercent());
	m_location.m_isPartOfFarmField.designateForHarvestIfPartOfField(m_plantSpecies);
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
	if(m_hasFluid && m_location.m_exposedToSky == m_plantSpecies.growsInSunLight && m_temperatureEvent.empty() && getPercentFoliage() >= Config::minimumPercentFoliageForGrow);
	{
		if(m_growthEvent.empty())
		{
			uint32_t delay = (m_percentGrown == 0 ?
					m_plantSpecies.stepsTillFullyGrown :
					((m_plantSpecies.stepsTillFullyGrown * m_percentGrown) / 100u));
			m_growthEvent.schedule(delay, *this);
		}
	}
	else
	{
		if(!m_growthEvent.empty())
		{
			m_percentGrown = getGrowthPercent();
			m_growthEvent.unschedule();
		}
	}
}
uint32_t Plant::getGrowthPercent() const
{
	uint32_t output = m_percentGrown;
	if(!m_growthEvent.empty())
	{
		if(m_percentGrown != 0)
			output += (m_growthEvent->percentComplete() * (100u - m_percentGrown)) / 100u;
		else
			output = m_growthEvent->percentComplete();
	}
	return output;
}
uint32_t Plant::getRootRange() const
{
	return m_plantSpecies.rootRangeMin + ((m_plantSpecies.rootRangeMax - m_plantSpecies.rootRangeMin) * growthPercent()) / 100;
}
uint32_t Plant::getPercentFoliage() const
{
	uint32_t output = m_percentFoliage;
	if(!m_foliageGrowthEvent.empty())
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
	uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, growthPercent());
	return util::scaleByPercent(maxForGrowth, getPercentFoliage());
}
void Plant::removeFoliageMass(uint32_t mass)
{
	uint32_t maxFoliageForType = util::scaleByPercent(m_plantSpecies.adultMass, Config::percentOfPlantMassWhichIsFoliage);
	uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, growthPercent());
	uint32_t percentRemoved = ((maxForGrowth - mass) / maxForGrowth) * 100u;
	m_percentFoliage = getPercentFoliage();
	assert(m_percentFoliage >= percentRemoved);
	m_percentFoliage -= percentRemoved;
	m_foliageGrowthEvent->maybeUnschedule();
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
void HasPlant::addPlant(const PlantSpecies& plantSpecies, uint32_t growthPercent = 0)
{
	assert(m_plant == nullptr);
	m_block.m_location->m_area->m_hasPlants.emplace(plantSpecies, growthPercent);
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
void HasPlants::emplace(Block* location, const PlantSpecies& species, uint32_t percentGrowth)
{
	m_plants.emplace_back(location, species, percentGrowth);
}
void HasPlants::erase(Plant& plant)
{
	std::erase_if(m_plants, [&](Plant& p) { return &p == &plant; });
}
