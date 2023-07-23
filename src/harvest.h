#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "path.h"
#include "config.h"

class HarvestObjective;

class HarvestEvent : public ScheduledEvent
{
	HarvestObjective& m_harvestObjective;
	HarvestEvent(uint32_t step, HarvestObjective& ho) : ScheduledEvent(step), m_harvestObjective(ho) {}
	void execute();
	Plant* getPlant();
	~HarvestEvent();
};
class HarvestThreadedTask : public ThreadedTask
{
	HarvestObjective& m_harvestObjective;
	std::vector<Block*> m_result;
	HarvestThreadedTask(HarvestObjective& ho) : m_harvestObjective(ho) {}
	void readStep();
	void writeStep();
};
class HarvestObjectiveType : public ObjectiveType
{
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class HarvestObjective : public Objective
{
	Actor& m_actor;
public:
	HarvestObjective(Actor& a) : Objective(Config::harvestPriority), m_actor(a) {}
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HasThreadedTask<HarvestThreadedTask> m_threadedTask;
	void execute();
	bool canHarvestAt(Block& block) const;
	friend class HarvestEvent;
	friend class HarvestThreadedTask;
};
