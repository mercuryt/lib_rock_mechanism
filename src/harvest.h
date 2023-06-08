#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include <memory>

template<class HarvestObjective, class Plant>
class HarvestEvent : ScheduledEvent
{
	HarvestObjective& m_harvestObjective;
	HarvestEvent(uint32_t step, HarvestObjective& ho) : ScheduledEvent(step), m_harvestObjective(ho) {}
	void execute()
	{
		Plant* plant = getPlant();
		auto& actor = m_harvestObjective.m_actor;
		if(plant == nullptr)
			actor.actionComplete();
		
		uint32_t maxHarvestItemsCanCarry = (actor.m_carryWeight - actor.m_equipmentSet.m_totalMass) / plant.m_plantType.harvestItemType.mass;
		uint32_t numberItemsHarvested = std::min(maxHarvestItemsCanCarry, plant.numberOfHarvestableItems());
		//TODO: apply horticulture skill.
		plant.harvest(numberItemsHarvested);
		if(actor.m_genericItems.contains(plant.m_plantType.harvestItemType))
			actor.m_genericItems[plant.m_plantType.harvestItemType] += numberItemsHarvested;
		else
			actor.m_genericItems[plant.m_plantType.harvestItemType] = numberItemsHarvested;
	}
	Plant* getPlant()
	{
		auto& block = *m_harvestObjective.m_actor.m_location;
		for(Plant* p : block.m_plants)
			if(p.readyToHarvest())
				return p;
		if(plant == nullptr)
			for(Block* adjacent : block.m_adjacentsVector)
				for(Plant* p : block.m_plants)
					if(p.readyToHarvest())
						return p;
		return nullptr;
	}
	~HarvestEvent() { m_harvestObjective.m_harvestEvent = nullptr; }
}

template<class HarvestObjective>
class HarvestThreadedTask : ThreadedTask
{
	HarvestObjective& m_harvestObjective;
	HarvestThreadedTask(HarvestObjective& ho) : m_harvestObjective(ho) {}
	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return block->anyoneCanEnterEver() && block->canEnterEver(m_harvestObjective.m_actor);
		}
		auto destinationCondition = [&](Block* block)
		{
			m_harvestObjective.canHarvestAt(*block);
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_drinkObjective.m_animal.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
			m_harvestObjective.m_actor.CannotFulfillObjective();
		else
			m_harvestObjective.m_actor.setDestination(m_result);
			
	}
}
template<class Actor, class Plant>
class HarvestObjective : Objective
{
	Actor& m_actor;
	Harvest(Actor& a) : m_actor(a) {}
	ScheduledEventWithPercent* m_harvestEvent;
	void execute()
	{
		if(canHarvestAt(*m_actor.m_location))
		{
			//TODO: apply horticultural skill.
			uint32_t step = s_step + Config::harvestEventDuration;
			std::unique_ptr<ScheduledEventWithPercent> event = std::make_unique<HarvestEvent>(step, *this);
			m_harvestEvent = event.get();
			m_actor.m_location->m_area->m_eventSchedule.schedule(std::move(event));
		}
		else
		{
			std::unique_ptr<ThreadedTask> task = std::make_unique<HarvestThreadedTask>(*this);
			m_actor.m_location->m_area->m_threadedTasks.push_back(std::move(task));
		}
	}
	bool canHarvestAt(Block& block) const
	{
		for(Plant& plant : block.m_plants)
			if(plant.m_readyToHarvest)
				return true;
		for(Block* adjacent : block.m_adjacentsVector)
			for(Plant& plant : adjacent->m_plants)
				if(plant.m_readyToHarvest)
					return true;
		return false;
	}
}
