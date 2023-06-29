#pragma once

#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "path.h"
#include "plant.h"

#include <memory>
#include <vector>

class GivePlantsFluidEvent : ScheduledEventWithPercent
{
	GivePlantsFluidObjective& m_objective;
	GivePlantsFluidEvent(uint32_t step, GivePlantsFluidObjective& gpfo) : ScheduledEvent(step), m_objective(gpfo) { }
	void execute()
	{
		assert(m_objective.m_actor.m_hasFluids.hasFluidType(m_objective.m_plant->plantType.fluidType));
		uint32_t quantity = std::min(m_objective.m_plant.getFluidDrinkVolume(), m_objective.m_actor.m_hasItems.getFluidVolume());
		m_objective.m_plant.m_needsFluid.drink(quantity);
		m_objective.m_actor.m_hasItems.removeFluidVolume(quantity);
		m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
	}
	~GivePlantsFluidEvent() { m_objective.m_event.clearPointer(); }
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid.
class GivePlantsFluidThreadedTask : ThreadedTask
{
	GivePlantsFluidObjective& m_objective;
	std::vector<Block*> m_result;
	GivePlantsFluidThreadedTask(GivePlantsFluidObjective* gpfo) : m_objective(gpfo) { }
	void readStep()
	{
		if(m_objective.m_plant == nullptr)
		{
			auto condition = [&](Block* block)
			{
				return block->m_hasDesignations.contains(m_objective.m_actor.getPlayer(), BlockDesignation::GivePlantFluid);
			};
			m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
			return;
		}
		if(m_objective.m_actor.m_hasItems.hasContainerWithFluidType(m_objective.m_plant->m_plantType.m_fluidType))
		{
			if(m_objective.m_actor.m_location == m_objective.m_plant->m_location)
				m_objective.m_givePlantsFluidEvent.schedule(::s_step + Config::givePlantsFluidEventDelaySteps, m_objective);
			else
				m_result = path::getForActor(m_objective.m_actor, *m_plant.m_location);
		}
		else
		{
			if(m_objective.m_actor.m_hasItems.hasEmptyWaterProofContainer())
			{
				auto condition = [&](Block* block)
				{
					return m_objective.canFillAt(block);
				};
				m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
			}
			else
			{
				auto condition [&](Block* block)
				{
					return block.m_hasItems.hasEmptyContainerCarryableBy(m_objective.m_actor) || block.m_hasItems.hasEmptyContainerCarryableByWithFluidType(m_objective.m_actor, m_objective.m_plant.m_plantType.fluidType);
				};
				m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
			}
		}
	}
	void writeStep()
	{
		if(m_result.empty() && m_objective.m_givePlantsFluidEvent.empty())
			m_objective.m_actor.m_hasObjectives.cannotCompleteObjective(m_objective);
		else
		{
			assert(m_objective.m_givePlantsFluidEvent.empty());
			if(!m_result.empty())
				m_objective.m_actor.setPath(m_result);
		}
	}
};
class GivePlantsFluidObjectiveType : ObjectiveType
{
	bool canBeAssigned(Actor& actor)
	{
		return actor.m_location->m_area->m_hasFarmFields.hasGivePlantsFluidDesignations();
	}
	std::unique_ptr<Objective> makeFor(Actor& actor)
	{
		return std::make_unique<GivePlantsFluidObjective>(actor);
	}
};
class GivePlantsFluidObjective : Objective
{
	Actor& m_actor;
	Plant* m_plant;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
	HasThreadedTask<GivePlantsFluidThreadedTask> m_threadedTask;
	GivePlantsFluidObjective(Actor& a ) : m_actor(a), m_plant(nullptr) { }
	void execute()
	{
		if(m_plant == nullptr)
		{
			m_plant = m_actor.m_location->m_area->m_hasFarmFields.getHighestPriorityPlantForGiveFluid(m_actor.getPlayer());
			if(m_plant != nullptr)
				execute();
			else
				if(m_actor.m_location->m_hasDesignations.contains(m_actor.getPlayer(), BlockDesignation::GivePlantFluid))
				{
					Plant& plant = &m_actor.m_location.m_hasPlant.get();
					if(m_actor.m_hasItems.hasContainerWithFluidType(m_plant.getFluidType()))
					{
						m_plant = plant;
						execute();
					}
					else
						m_threadedTask.create(*this);
				}
				else
				// Find a plant to give fluid to.
					m_threadedTask.create(*this);
		}
		else
		{
			if(m_actor.m_hasItems.hasContainerWithFluidType(m_plant.getFluidType()))
			{
				if(m_actor.m_location == m_plant->m_location)
					m_event.schedule(::step + Config::givePlantsFluidDelaySteps, *this);
				else
					m_actor.setDestination(*m_plant.location);
			}
			else
				// Find full container, empty contaner or fluid to fill empty container. 
				m_threadedTask.create(*this);
		}
	}
	bool canFillAt(Block& block) const
	{
		return getAdjacentBlockToFillAt(block) != nullptr || canFillItemAt(block);
	}
	Block* getAdjacentBlockToFillAt(Block& block) const
	{
		for(Block* adjacent : block.m_adjacentsVector)
			if(adjacent->m_fluids.contains(m_actor.getFluidType()))
				return adjacent;
		return nullptr;
	}
	bool canFillItemAt(Block* block) const
	{
		return getItemToFillFromAt(block) != nullptr;
	}
	Item* getItemToFillFromAt(Block* block) const
	{
		assert(m_actor.getFluidType() != nullptr);
		for(Item* item : block.m_hasItems.get())
			if(item->containsItemsOrFluids.getFluidType() == m_actor.getFluidType())
				return &item;
	}
};
