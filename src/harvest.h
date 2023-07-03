#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "path.h"

class HarvestEvent : ScheduledEvent
{
	HarvestObjective& m_harvestObjective;
	HarvestEvent(uint32_t step, HarvestObjective& ho) : ScheduledEvent(step), m_harvestObjective(ho) {}
	void execute();
	Plant* getPlant();
	~HarvestEvent();
};
class HarvestThreadedTask : ThreadedTask
{
	HarvestObjective& m_harvestObjective;
	HarvestThreadedTask(HarvestObjective& ho) : m_harvestObjective(ho) {}
	void readStep();
	void writeStep();
};
class HarvestObjectiveType : ObjectiveType
{
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class HarvestObjective : Objective
{
	Actor& m_actor;
	Harvest(Actor& a) : m_actor(a) {}
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HasThreadedTask<HarvestThreadedTask> m_threadedTask;
	void execute();
	bool canHarvestAt(Block& block) const;
};
