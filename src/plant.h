#pragma once

#include "eventSchedule.h"
#include "util.h"

#include <memory>

template<class FluidType>
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
};

template<class Plant>
class PlantGrowthEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantGrowthEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.m_percentGrown = 100;
		if(m_plant.m_plantType.annual)
			m_plant.setReadyToHarvest();
	}
	~PlantGrowthEvent() { m_plant.m_growthEvent = nullptr; }
};
template<class Plant>
class PlantFoliageGrowthEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantFoliageGrowthEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute(){ m_plant.foliageGrowth(); }
	~PlantFoliageGrowthEvent(){ m_plant.m_foliageGrowthEvent = nullptr; }
}
template<class Plant>
class PlantEndOfHarvestEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantEndOfHarvestEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.endOfHarvest(); }
	~PlantEndOfHarvestEvent() { m_plant.m_endOfHarvestEvent = nullptr; }
};
template<class Plant>
class PlantFluidEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantFluidEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.setMaybeNeedsFluid(); }
	~PlantFluidEvent() { m_plant.m_fluidEvent = nullptr; }
};
template<class Plant>
class PlantTemperatureEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantTemperatureEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.die(); }
	~PlantTemperatureEvent() { m_plant.m_temperatureEvent = nullptr; }
};
template<class DerivedPlant, class PlantType, class Block>
class BasePlant
{
public:
	Block& m_location;
	Block* m_fluidSource;
	const PlantType& m_plantType;
	ScheduledEventWithPercent* m_growthEvent;
	ScheduledEventWithPercent* m_fluidEvent;
	ScheduledEventWithPercent* m_temperatureEvent;
	ScheduledEventWithPercent* m_endOfHarvestEvent;
	ScheduledEventWithPercent* m_foliageGrowthEvent;
	uint32_t m_percentGrown;
	bool m_readyToHarvest;
	bool m_hasFluid;
	uint32_t m_percentFoliage;

	BasePlant(Block& l, const PlantType& pt, uint32_t pg = 0) : m_location(l), m_fluidSource(nullptr), m_plantType(pt), m_growthEvent(nullptr), m_fluidEvent(nullptr), m_temperatureEvent(nullptr), m_endOfHarvestEvent(nullptr), m_foliageGrowthEvent(nullptr), m_percentGrown(pg), m_readyToHarvest(false), m_hasFluid(true), m_percentFoliage(100)
	{
		assert(m_location.plantTypeCanGrow(m_plantType));
		m_location.m_plants.push_back(&derived());
		createFluidEvent(m_plantType.stepsNeedsFluidFrequency);
		updateGrowingStatus();
	}
	DerivedPlant& derived(){ return static_cast<DerivedPlant&>(*this); }
	void die()
	{
		if(m_growthEvent != nullptr)
			m_growthEvent->cancel();
		if(m_fluidEvent != nullptr)
			m_fluidEvent->cancel();
		if(m_temperatureEvent != nullptr)
			m_temperatureEvent->cancel();
		if(m_endOfHarvestEvent != nullptr)
			m_endOfHarvestEvent->cancel();
		if(m_foliageGrowthEvent != nullptr)
			m_foliageGrowthEvent->cancel();
		derived().onDie();
	}
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature)
	{
		(void)oldTemperature;
		if(newTemperature >= m_plantType.minimumGrowingTemperature && newTemperature <= m_plantType.maximumGrowingTemperature)
		{
			if(m_temperatureEvent != nullptr)
				m_temperatureEvent->cancel();
		}
		else
		{
			if(m_temperatureEvent == nullptr)
			{
				uint32_t step = s_step + m_plantType.stepsTillDieFromTemperature;
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantTemperatureEvent<DerivedPlant>>(step, derived());
				m_temperatureEvent = event.get();
				m_location.m_area->m_eventSchedule.schedule(std::move(event));
			}
		}
		updateGrowingStatus();

	}
	void setHasFluidForNow()
	{
		m_hasFluid = true;
		if(m_fluidEvent != nullptr)
		{
			m_fluidEvent->cancel();
			m_fluidEvent = nullptr;
		}
		createFluidEvent(m_plantType.stepsNeedsFluidFrequency);
		updateGrowingStatus();
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
		}
		createFluidEvent(stepsTillNextFluidEvent);
		updateGrowingStatus();
	}
	void createFluidEvent(uint32_t delay)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantFluidEvent<DerivedPlant>>(s_step + delay, derived());
		m_fluidEvent = event.get();
		m_location.m_area->m_eventSchedule.schedule(std::move(event));
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
			setReadyToHarvest();
	}
	void setReadyToHarvest()
	{
		m_readyToHarvest = true;
		uint32_t step = s_step + m_plantType.stepsTillEndOfHarvest;
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantEndOfHarvestEvent<DerivedPlant>>(step, derived());
		m_endOfHarvestEvent = event.get();
		m_location.m_area->m_eventSchedule.schedule(std::move(event));
	}
	void endOfHarvest()
	{
		m_readyToHarvest = false;
		derived().onEndOfHarvest();
		if(m_plantType.annual)
			die();
	}
	void updateGrowingStatus()
	{
		if(m_hasFluid && m_location.m_exposedToSky == m_plantType.growsInSunLight && m_temperatureEvent == nullptr && getPercentFoliage() >= Config::minimumPercentFoliageForGrow);
		{
			if(m_growthEvent == nullptr)
			{
				uint32_t step = s_step + (m_percentGrown == 0 ?
						m_plantType.stepsTillFullyGrown :
						((m_plantType.stepsTillFullyGrown * m_percentGrown) / 100u));
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantGrowthEvent<DerivedPlant>>(step, derived());
				m_growthEvent = event.get();
				m_location.m_area->m_eventSchedule.schedule(std::move(event));
			}
		}
		else
		{
			if(m_growthEvent != nullptr)
			{
				m_percentGrown = growthPercent();
				m_growthEvent->cancel();
			}
		}
	}
	uint32_t getGrowthPercent() const
	{
		uint32_t output = m_percentGrown;
		if(m_growthEvent != nullptr)
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
		if(m_foliageGrowthEvent != nullptr)
		{
			if(m_percentFoliage != 0)
				output += (m_growthEvent->percentComplete() * (100u - m_percentFoliage)) / 100u;
			else
				output = m_growthEvent->percentComplete();
		}
		return output;
	}
	uint32_t getFoliageMass() const
	{
		uint32_t maxFoliageForType = util::scaleByPercent(m_plantType.adultMass, Config::percentOfPlantMassWhichIsFoliage);
		uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, growthPercent());
		return util::scaleByPercent(maxForGrowth, getPercentFoliage();
	}
	void removeFoliageMass(uint32_t mass)
	{
		uint32_t maxFoliageForType = util::scaleByPercent(m_plantType.adultMass, Config::percentOfPlantMassWhichIsFoliage);
		uint32_t maxForGrowth = util::scaleByPercent(maxFoliageForType, growthPercent());
		uint32_t percentRemoved = ((maxForGrowth - mass) / maxForGrowth) * 100u;
		m_percentFoliage = getPercentFoliage();
		assert(m_percentFoliage >= percentRemoved);
		m_percentFoliage -= percentRemoved;
		if(m_foliageGrowthEvent != nullptr)
			m_foliageGrowthEvent->cancel();
		makeFoliageGrowthEvent();
		updateGrowingStatus();
	}
	void makeFoliageGrowthEvent()
	{
		uint32_t step = s_step + util::scaleByInversePercent(m_plantType.stepsTillFoliageGrowsFromZero, m_percentFoliage);
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantFoliageGrowthEvent>(step, *this);
		m_foliageGrowthEvent = event.get();
		m_location->m_area->m_eventSchedule.schedule(event);
	}
	void foliageGrowth()
	{
		m_percentFoliage = 100;
		updateGrowingStatus();
	}
	// User provided code.
	void onDie();
	void onEndOfHarvest();
};
