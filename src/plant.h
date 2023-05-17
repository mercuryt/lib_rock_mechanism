#pragma once

#include "eventSchedule.h"
#include "randomUtil.h"
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
	const uint32_t stepsTillSpawn;
	const uint32_t dayOfYearForHarvest;
	const uint32_t maximumSpawn;
	const uint32_t minimumSpawn;
	const uint32_t maximumSpawnDistance;
	const uint32_t rootRangeMax;
	const uint32_t rootRangeMin;
	const FluidType* fluidType;
};

static std::list<PlantType> s_plantTypes;

template<class Block> class Plant;

const PlantType* registerPlantType(std::string n, bool a, uint32_t maxT, uint32_t minT, uint32_t stdft, uint32_t snff, uint32_t scswf, uint32_t stfg, bool gisl, uint32_t sts, uint32_t doyfh, uint32_t maxSpawn, uint32_t minSpawn, uint32_t msd, uint32_t rrMax, uint32_t rrMin, const FluidType* ft)
{
	s_plantTypes.emplace_back(n, a, maxT, minT, stdft, snff, scswf, stfg, gisl, sts, doyfh, maxSpawn, minSpawn, msd, rrMax, rrMin, ft);
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
class PlantSpawnEvent : public ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantSpawnEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.doSpawn(); }
	~PlantSpawnEvent() { m_plant.m_spawnEvent = nullptr; }
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
	ScheduledEventWithPercent* m_spawnEvent;
	uint32_t m_percentGrown;
	bool m_readyToHarvest;
	bool m_hasFluid;

	Plant(Block& l, const PlantType* pt, uint32_t pg = 0) : m_location(l), m_fluidSource(nullptr), m_plantType(pt), m_growthEvent(nullptr), m_fluidEvent(nullptr), m_temperatureEvent(nullptr), m_spawnEvent(nullptr), m_percentGrown(pg), m_readyToHarvest(false), m_hasFluid(true) 
	{
		assert(m_location.plantTypeCanGrow(m_plantType));
		createFluidEvent(m_plantType->stepsNeedsFluidFrequency);
		updateGrowingStatus();
	}
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature)
	{
		(void)oldTemperature;
		if(newTemperature > m_plantType->minimumGrowingTemperature && newTemperature < m_plantType->maximumGrowingTemperature)
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
		m_spawnEvent = event.get();
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
		uint32_t step = s_step + m_plantType->stepsTillSpawn;
		std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<PlantSpawnEvent<Block>>(step, *this);
		m_spawnEvent = event.get();
		m_location.m_area->m_eventSchedule.schedule(std::move(event));
	}
	void doSpawn()
	{
		uint32_t numberOfSpawn = randomUtil::getInRange(m_plantType->minimumSpawn, m_plantType->maximumSpawn);
		std::unordered_set<Block*> inRangeSet = util<Block>::collectAdjacentsInRange(m_plantType->maximumSpawnDistance, m_location);
		std::vector<Block*> candidates;
		for(Block*  block : inRangeSet)
			if(block->plantTypeCanGrow(m_plantType))
				candidates.push_back(block);
		std::shuffle(candidates.begin(), candidates.end(), randomUtil::getRng());
		for(uint32_t i = 0; i < candidates.size() && i < numberOfSpawn; ++i)
			m_location.m_area->m_plants.emplace_back(*candidates[i], m_plantType);
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
	void die();
};
