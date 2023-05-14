#pragma once
#include <memory>

template<Block>
class PlantGrowthEvent : ScheduledEventWithPercent
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
	void cancel()
	{
		m_plant.m_growthEvent = nullptr;
		ScheduledEventWithPercent::cancel();
	}
}
template<Block>
class PlantSpawnEvent : ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantSpawnEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.doSpawn();
	}
	void cancel()
	{
		m_plant.m_spawnEvent = nullptr;
		ScheduledEventWithPercent::cancel();
	}
}
template<Block>
class PlantFluidEvent : ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantFluidEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.setMaybeNeedsFluid();
	}
	void cancel()
	{
		m_plant.m_fluidEvent = nullptr;
		ScheduledEventWithPercent::cancel();
	}
}
template<Block>
class PlantTemperatureEvent : ScheduledEventWithPercent
{
	Plant<Block>& m_plant;
public:
	PlantTemperatureEvent(uint32_t step, Plant<Block>& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.die();
	}
	void cancel()
	{
		m_plant.m_temperatureEvent = nullptr;
		ScheduledEventWithPercent::cancel();
	}
}

template<class Block>
class Plant
{
	Block& m_location;
	Block* m_fluidSource;
	const PlantType* m_plantType;
	ScheduledEvent* m_growthEvent;
	ScheduledEvent* m_fluidEvent;
	ScheduledEvent* m_temperatureEvent;
	ScheduledEvent* m_spawnEvent;
public:
	uint32_t m_percentGrown;
	bool m_readyToHarvest;
	bool m_hasFluid;

	Plant(Block& l, const PlantType* pt, uint32_t pg = 0) : m_location(l), m_plantType(pt), m_percentGrown(pg), m_readyToHarvest(false), m_hasFluid(false) {}
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature);
	void setHasFluidForNow()
	{
		m_hasFluid = true;
		if(m_fluidEvent != nullptr)
		{
			m_fluidEvent.cancel();
			m_fluidEvent = nullptr;
		}
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
			stepsTillNextFluidEvent = m_plantType->stepsCanSurviveWithoutFluid;
		}
		uint32_t step = s_step + m_plantType->stepsCanSurviveWithoutFluid;
		std::unique_ptr<ScheduledEvent> event = std::make_unique<PlantFluidEvent<Block>>(step, *this);
		m_spawnEvent = event.get();
		m_location->m_area.m_eventSchedule.schedule(std::move(event));
		updateGrowingStatus();
	}
	void setExposedToSky(bool exposedToSky);
	void setDayOfYear(uint32_t dayOfYear);
	void setReadyToHarvest()
	{
		m_readyToHarvest = true;
		uint32_t step = s_step + m_plantType->stepsTillSpawn;
		std::unique_ptr<ScheduledEvent> event = std::make_unique<PlantSpawnEvent<Block>>(step, *this);
		m_spawnEvent = event.get();
		m_location->m_area.m_eventSchedule.schedule(std::move(event));
	}
	void doSpawn()
	{
		uint32_t numberOfSpawn = random::getInRange(m_plantType.minimumSpawn, m_plantType.maximumSpawn);
		std::unordered_set<Block*> inRangeSet = util::collectAdjacentsInRange(getRootRange(), m_location);
		std::vector<Block*> candidates;
		for(Block*  block : inRangeSet)
			if(block[0] != nullptr && block.plantTypeCanGrow(m_plantType))
				candidates.push_back(block);
		std::shuffle(candidates.begin(), candidates.end(), random::getRng());
		for(uint32_t i = 0; i < candidates.end() && i < numberOfSpawn; ++i)
			m_location.m_area.m_plants.emplace_back(*candidate, m_plantType);
	}
	void updateGrowingStatus()
	{
		if(m_hasFluid && m_location.m_exposedToSky == m_plantType.m_growsInSunLight && m_temperatureEvent == nullptr)
		{
			if(m_growthEvent == nullptr)
			{
				uint32_t step = s_step + ((m_plantType->m_stepsTillFullyGrown * m_percentGrown) / 100u);
				std::unique_ptr<ScheduledEvent> event = std::make_unique<PlantGrowthEvent>(step, *this);
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
	uint32_t growhPercent() const
	{
		uint32_t output = m_percentGrown;
		if(m_growthEvent != nullptr)
			output += m_growthEvent->percentComplete();
		return output;
	}
}
