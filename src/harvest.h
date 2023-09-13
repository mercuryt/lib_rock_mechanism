#pragma once

#include "objective.h"
#include "config.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"

class HarvestThreadedTask;
class HarvestEvent;
class Block;
class Plant;
class Actor;

class HarvestObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveId getObjectiveId() const { return ObjectiveId::Harvest; }
};
class HarvestObjective final : public Objective
{
	Actor& m_actor;
public:
	HarvestObjective(Actor& a);
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HasThreadedTask<HarvestThreadedTask> m_threadedTask;
	void execute();
	void cancel();
	void delay() { cancel(); }
	std::string name() const { return "harvest"; }
	ObjectiveId getObjectiveId() const { return ObjectiveId::Harvest; }
	bool canHarvestAt(Block& block) const;
	friend class HarvestEvent;
	friend class HarvestThreadedTask;
};
class HarvestEvent final : public ScheduledEventWithPercent
{
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(Step delay, HarvestObjective& ho);
	void execute();
	void clearReferences();
	Plant* getPlant();
};
class HarvestThreadedTask final : public ThreadedTask
{
	HarvestObjective& m_harvestObjective;
	std::vector<Block*> m_result;
public:
	HarvestThreadedTask(HarvestObjective& ho);
	void readStep();
	void writeStep();
	void clearReferences();
};
