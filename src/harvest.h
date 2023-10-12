#pragma once

#include "objective.h"
#include "config.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "findsPath.h"
#include "types.h"

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
	void execute();
	void cancel();
	void delay() { cancel(); }
	std::string name() const { return "harvest"; }
	ObjectiveId getObjectiveId() const { return ObjectiveId::Harvest; }
	bool blockContainsHarvestablePlant(const Block& block) const;
	friend class HarvestEvent;
};
class HarvestEvent final : public ScheduledEventWithPercent
{
	HarvestObjective& m_harvestObjective;
	Block& m_block;
public:
	HarvestEvent(Step delay, HarvestObjective& ho, Block& b);
	void execute();
	void clearReferences();
};
