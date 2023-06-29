#pragma once

#include "eventSchedule.h"
#include "util.h"

class BasePlantType
{
public:
	const std::string name;
	const bool annual;
	const uint32_t maximumGrowingTemperature;
	const uint32_t minimumGrowingTemperature;
	const uint32_t stepsTillDieFromTemperature;
	const uint32_t stepsNeedsFluidFrequency;
	const uint32_t stepsTillDieWithoutFluid;
	const uint32_t stepsTillFullyGrown;
	const bool growsInSunLight;
	const uint32_t dayOfYearForHarvest;
	const uint32_t stepsTillEndOfHarvest;
	const uint32_t rootRangeMax;
	const uint32_t rootRangeMin;
	const uint32_t adultMass;
	const FluidType& fluidType;
	const ItemType* fruitType; // May be nullptr.
	const uint32_t fruitQuantityWhenFullyGrown;
};

class PlantGrowthEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantGrowthEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.m_percentGrown = 100;
		if(m_plant.m_plantType.annual)
			m_plant.setQuantityToHarvest();
	}
	~PlantGrowthEvent() { m_plant.m_growthEvent.clearPointer(); }
};
class PlantFoliageGrowthEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantFoliageGrowthEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute(){ m_plant.foliageGrowth(); }
	~PlantFoliageGrowthEvent(){ m_plant.m_foliageGrowthEvent.clearPointer(); }
}
class PlantEndOfHarvestEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantEndOfHarvestEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.endOfHarvest(); }
	~PlantEndOfHarvestEvent() { m_plant.m_endOfHarvestEvent.clearPointer(); }
};
class PlantFluidEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantFluidEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.setMaybeNeedsFluid(); }
	~PlantFluidEvent() { m_plant.m_fluidEvent.clearPointer(); }
};
class PlantTemperatureEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantTemperatureEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.die(); }
	~PlantTemperatureEvent() { m_plant.m_temperatureEvent.clearPointer(); }
};
class BasePlant
{
public:
	Block& m_location;
	Block* m_fluidSource;
	const PlantType& m_plantType;
	HasScheduledEvent<PlantGrowthEvent> m_growthEvent;
	HasScheduledEvent<PlantFluidEvent> m_fluidEvent;
	HasScheduledEvent<PlantTemperatureEvent> m_temperatureEvent;
	HasScheduledEvent<PlantEndOfHarvestEvent> m_endOfHarvestEvent;
	HasScheduledEvent<PlantFoliageGrowthEvent> m_foliageGrowthEvent;
	uint32_t m_percentGrown;
	uint32_t m_quantityToHarvest;
	bool m_hasFluid;
	uint32_t m_percentFoliage;
	//TODO: Set max reservations to 1 to start, maybe increase later with size?
	Reserveable m_reservable;

	BasePlant(Block& l, const PlantType& pt, uint32_t pg = 0) : m_location(l), m_fluidSource(nullptr), m_plantType(pt), m_percentGrown(pg), m_quantityToHarvest(0), m_hasFluid(true), m_percentFoliage(100), m_reservable(1)
	{
		assert(m_location.plantTypeCanGrow(m_plantType));
		m_location.m_plants.push_back(&derived());
		m_fluidEvent.schedule(s_step + m_plantType.stepsNeedsFluidFrequency, derived());
		updateGrowingStatus();
	}
	DerivedPlant& derived(){ return static_cast<DerivedPlant&>(*this); }
	void die()
	{
		m_growthEvent.maybeUnschedule();
		m_fluidEvent.maybeUnschedule();
		m_temperatureEvent.maybeUnschedule();
		m_endOfHarvestEvent.maybeUnschedule();
		m_foliageGrowthEvent.maybeUnschedule();
		derived().onDie();
	}
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature)
	{
		(void)oldTemperature;
		if(newTemperature >= m_plantType.minimumGrowingTemperature && newTemperature <= m_plantType.maximumGrowingTemperature)
		{
			if(!m_temperatureEvent.empty())
				m_temperatureEvent->unschedule();
		}
		else
			if(m_temperatureEvent.empty())
				m_temperatureEvent.schedule(s_step + m_plantType.stepsTillDieFromTemperature, derived());
		updateGrowingStatus();
	}
	void setHasFluidForNow()
	{
		m_hasFluid = true;
		if(!m_fluidEvent.empty())
			m_fluidEvent->unschedule();
		m_fluidEvent.schedule(s_step + m_plantType.stepsNeedsFluidFrequency, derived());
		updateGrowingStatus();
		m_location.m_isPartOfFarmField.removeAllGiveFluidDesignations();
	}
	void setMaybeNeedsFluid()
	{
		uint32_t stepsTillNextFluidEvent;
		if(hasFluidSource())
		{
			m_hasFluid = true;
			stepsTillNextFluidEvent = m_plantType.stepsNeedsFluidFrequency;
		}
		else if(!m_hasFluid)
		{
			die();
			return;
		}
		else // Needs fluid, stop growing and set death timer.
		{
			m_hasFluid = false;
			stepsTillNextFluidEvent = m_plantType.stepsTillDieWithoutFluid;
			m_location.m_isPartOfFarmField.designateForGiveFluidIfPartOfFarmField;
		}
		m_fluidEvent.schedule(s_step + stepsTillNextFluidEvent, derived());
		updateGrowingStatus();
	}
	bool hasFluidSource() const
	{
		if(m_fluidSource != nullptr && m_fluidSource->m_fluids.contains(&m_plantType.fluidType))
			return true;
		m_fluidSource = nullptr;
		for(Block* block : util<Block>::collectAdjacentsInRange(getRootRange(), m_location))
			if(block->m_fluids.contains(&m_plantType.fluidType))
			{
				m_fluidSource = block;
				return true;
			}
		return false;
	}
	void setDayOfYear(uint32_t dayOfYear)
	{
		if(!m_plantType.annual && dayOfYear == m_plantType.dayOfYearForHarvest)
			setQuantityToHarvest();
	}
	void setQuantityToHarvest()
	{
		m_endOfHarvestEvent.schedule(s_step + m_plantType.stepsTillEndOfHarvest, derived());
		m_quantityToHarvest = util::scaleByPercent(m_plantType.fruitQuantityWhenFullyGrown, getGrowthPercent());
		m_location.m_isPartOfFarmField.designateForHarvestIfPartOfField(m_plantType);
	}
	void harvest(uint32_t quantity)
	{
		assert(quantity >= m_quantityToHarvest);
		m_quantityToHarvest -= quantity;
		if(m_quantityToHarvest == 0)
			endOfHarvest();
	}
	void endOfHarvest()
	{
		m_location.m_isPartOfFarmField.removeAllHarvestDesignations();
		if(m_plantType.annual)
			die();
		else
			m_quantityToHarvest = 0;
	}
	void updateGrowingStatus()
	{
		if(m_hasFluid && m_location.m_exposedToSky == m_plantType.growsInSunLight && m_temperatureEvent.empty() && getPercentFoliage() >= Config::minimumPercentFoliageForGrow);
		{
			if(m_growthEvent.empty())
			{
				uint32_t step = s_step + (m_percentGrown == 0 ?
						m_plantType.stepsTillFullyGrown :
						((m_plantType.stepsTillFullyGrown * m_percentGrown) / 100u));
				m_growthEvent.schedule(step, derived());
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
	uint32_t getGrowthPercent() const
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
	uint32_t getRootRange() const
	{
		return m_plantType.rootRangeMin + ((m_plantType.rootRangeMax - m_plantType.rootRangeMin) * growthPercent()) / 100;
	}
	uint32_t getPercentFoliage() const
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
	uint32_t getFoliageMass() const
	{
		uint32_t maxFoliageForType = util::scaleByPercent(m_plantType.adultMass, Config::percentOfPlantMassWhichIsFoliage);
		uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, growthPercent());
		return util::scaleByPercent(maxForGrowth, getPercentFoliage());
	}
	void removeFoliageMass(uint32_t mass)
	{
		uint32_t maxFoliageForType = util::scaleByPercent(m_plantType.adultMass, Config::percentOfPlantMassWhichIsFoliage);
		uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, growthPercent());
		uint32_t percentRemoved = ((maxForGrowth - mass) / maxForGrowth) * 100u;
		m_percentFoliage = getPercentFoliage();
		assert(m_percentFoliage >= percentRemoved);
		m_percentFoliage -= percentRemoved;
		m_foliageGrowthEvent->maybeUnschedule();
		makeFoliageGrowthEvent();
		updateGrowingStatus();
	}
	void makeFoliageGrowthEvent()
	{
		uint32_t step = s_step + util::scaleByInversePercent(m_plantType.stepsTillFoliageGrowsFromZero, m_percentFoliage);
		m_foliageGrowthEvent.schedule(step, *this);
	}
	void foliageGrowth()
	{
		m_percentFoliage = 100;
		updateGrowingStatus();
	}
};
// To be used by block.
class HasPlant
{
	Block& m_block;
	Plant* m_plant;
public:
	HasPlant(Block& b) : m_block(b) { }
	void addPlant(const PlantType& plantType, uint32_t growthPercent = 0)
	{
		assert(m_plant == nullptr);
		m_plant = m_block.m_location->m_area->m_hasPlants.emplace(plantType, growthPercent);
	}
	void clearPointer()
	{
		assert(m_plant != nullptr);
		m_plant = nullptr;
	}
	Plant& get()
	{
		return *m_plant;
	}
};
