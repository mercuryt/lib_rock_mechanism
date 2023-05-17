#pragma once

#include "eventSchedule.h"
#include "util.h"

#include <memory>

class PlantType
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
	const FluidType* fluidType;
};

static std::list<PlantType> s_plantTypes;

template<class Block> class Plant;

template<typename... Arguments>
const PlantType* registerPlantType(const Arguments&... arguments)
{
	s_plantTypes.emplace_back(arguments...);
	return &s_plantTypes.back();
}

template<class Block>
class PlantGrowthEvent : public ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantGrowthEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.m_percentGrown = 100;
		if(m_plant.m_plantType->annual)
			m_plant.setReadyToHarvest();
	}
	~PlantGrowthEvent() { m_plant.m_growthEvent = nullptr; }
};
template<class Block>
class PlantEndOfHarvestEvent : public ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantEndOfHarvestEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.endOfHarvest(); }
	~PlantEndOfHarvestEvent() { m_plant.m_endOfHarvestEvent = nullptr; }
};
template<class Block>
class PlantFluidEvent : public ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantFluidEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.setMaybeNeedsFluid(); }
	~PlantFluidEvent() { m_plant.m_fluidEvent = nullptr; }
};
template<class Block>
class PlantTemperatureEvent : public ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantTemperatureEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.die(); }
	~PlantTemperatureEvent() { m_plant.m_temperatureEvent = nullptr; }
};
template<class Block>
class Plant
{
public:
	Block& m_location;
	Block* m_fluidSource;
	const PlantType* m_plantType;
	ScheduledEventWithPercent* m_growthEvent;
	ScheduledEventWithPercent* m_fluidEvent;
	ScheduledEventWithPercent* m_temperatureEvent;
	ScheduledEventWithPercent* m_endOfHarvestEvent;
	uint32_t m_percentGrown;
	bool m_readyToHarvest;
	bool m_hasFluid;

	Plant(Block& l, const PlantType* pt, uint32_t pg = 0) : m_location(l), m_fluidSource(nullptr), m_plantType(pt), m_growthEvent(nullptr), m_fluidEvent(nullptr), m_temperatureEvent(nullptr), m_endOfHarvestEvent(nullptr), m_percentGrown(pg), m_readyToHarvest(false), m_hasFluid(true) 
	{
		assert(m_location.plantTypeCanGrow(m_plantType));
		m_location.m_plants.push_back(this);
		createFluidEvent(m_plantType->stepsNeedsFluidFrequency);
		updateGrowingStatus();
	}
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
		onDie();
	}
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature)
	{
		(void)oldTemperature;
		if(newTemperature >= m_plantType->minimumGrowingTemperature && newTemperature <= m_plantType->maximumGrowingTemperature)
		{
			if(m_temperatureEvent != nullptr)
				m_temperatureEvent->cancel();
		}
		else
		{
			if(m_temperatureEvent == nullptr)
			{
				uint32_t step = s_step + m_plantType->stepsTillDieFromTemperature;
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantTemperatureEvent<Block>>(step, *this);
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
		createFluidEvent(m_plantType->stepsNeedsFluidFrequency);
		updateGrowingStatus();
	}
	void setMaybeNeedsFluid()
	{
		uint32_t stepsTillNextFluidEvent;
		if(hasFluidSource())
		{
			m_hasFluid = true;
			stepsTillNextFluidEvent = m_plantType->stepsNeedsFluidFrequency;
		}
		else if(!m_hasFluid)
		{
			die();
			return;
		}
		else // Needs fluid, stop growing and set death timer.
		{
			m_hasFluid = false;
			stepsTillNextFluidEvent = m_plantType->stepsTillDieWithoutFluid;
		}
		createFluidEvent(stepsTillNextFluidEvent);
		updateGrowingStatus();
	}
	void createFluidEvent(uint32_t delay)
	{
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantFluidEvent<Block>>(s_step + delay, *this);
		m_fluidEvent = event.get();
		m_location.m_area->m_eventSchedule.schedule(std::move(event));
	}
	void setDayOfYear(uint32_t dayOfYear)
	{
		if(!m_plantType->annual && dayOfYear == m_plantType->dayOfYearForHarvest)
			setReadyToHarvest();
	}
	void setReadyToHarvest()
	{
		m_readyToHarvest = true;
		uint32_t step = s_step + m_plantType->stepsTillEndOfHarvest;
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantEndOfHarvestEvent<Block>>(step, *this);
		m_endOfHarvestEvent = event.get();
		m_location.m_area->m_eventSchedule.schedule(std::move(event));
	}
	void endOfHarvest()
	{
		m_readyToHarvest = false;
		onEndOfHarvest();
		if(m_plantType->annual)
			die();
	}
	void updateGrowingStatus()
	{
		if(m_hasFluid && m_location.m_exposedToSky == m_plantType->growsInSunLight && m_temperatureEvent == nullptr)
		{
			if(m_growthEvent == nullptr)
			{
				uint32_t step = s_step + (m_percentGrown == 0 ?
						m_plantType->stepsTillFullyGrown :
						((m_plantType->stepsTillFullyGrown * m_percentGrown) / 100u));
				std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantGrowthEvent<Block>>(step, *this);
				m_growthEvent = event.get();
				m_location.m_area->m_eventSchedule.schedule(std::move(event));
			}
		}
		else
		{
			if(m_growthEvent != nullptr)
			{
				m_percentGrown += m_growthEvent->percentComplete();
				m_growthEvent->cancel();
				m_growthEvent = nullptr;
			}
		}
	}
	bool hasFluidSource()
	{
		if(m_fluidSource != nullptr && m_fluidSource->m_fluids.contains(m_plantType->fluidType))
			return true;
		m_fluidSource = nullptr;
		for(Block* block : util<Block>::collectAdjacentsInRange(getRootRange(), m_location))
			if(block->m_fluids.contains(m_plantType->fluidType))
			{
				m_fluidSource = block;
				return true;
			}
		return false;
	}
	uint32_t growthPercent() const
	{
		uint32_t output = m_percentGrown;
		if(m_growthEvent != nullptr)
			output += m_growthEvent->percentComplete();
		return output;
	}
	uint32_t getRootRange() const
	{
		return m_plantType->rootRangeMin + ((m_plantType->rootRangeMax - m_plantType->rootRangeMin) * growthPercent()) / 100;
	}
	// User provided code.
	void onDie();
	void onEndOfHarvest();
};
