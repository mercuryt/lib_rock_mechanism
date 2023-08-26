#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "path.h"
#include "config.h"
#include "eventSchedule.hpp"

class HarvestThreadedTask;
class HarvestEvent;

class HarvestObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
};
class HarvestObjective final : public Objective
{
	Actor& m_actor;
public:
	HarvestObjective(Actor& a) : Objective(Config::harvestPriority), m_actor(a), m_harvestEvent(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine()) {}
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HasThreadedTask<HarvestThreadedTask> m_threadedTask;
	void execute();
	void cancel() {}
	std::string name() { return "harvest"; }
	bool canHarvestAt(Block& block) const;
	friend class HarvestEvent;
	friend class HarvestThreadedTask;
};
class HarvestEvent final : public ScheduledEventWithPercent
{
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(Step delay, HarvestObjective& ho) : ScheduledEventWithPercent(ho.m_actor.getSimulation(), delay), m_harvestObjective(ho) {}
	void execute();
	void clearReferences();
	Plant* getPlant();
};
class HarvestThreadedTask final : public ThreadedTask
{
	HarvestObjective& m_harvestObjective;
	std::vector<Block*> m_result;
public:
	HarvestThreadedTask(HarvestObjective& ho) : ThreadedTask(ho.m_actor.getThreadedTaskEngine()), m_harvestObjective(ho) {}
	void readStep();
	void writeStep();
	void clearReferences();
};
