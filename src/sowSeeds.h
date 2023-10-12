#pragma once
#include "objective.h"
#include "plant.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "config.h"
#include "findsPath.h"

#include <memory>
#include <vector>

class SowSeedsEvent;
class SowSeedsObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveId getObjectiveId() const { return ObjectiveId::SowSeeds; }
};
class SowSeedsObjective final : public Objective
{
	Actor& m_actor;
	HasScheduledEvent<SowSeedsEvent> m_event;
	bool canSowSeedsAt(Block& location, Facing facing);
	Block* getBlockToSowAt(Block& location, Facing facing);
public:
	SowSeedsObjective(Actor& a);
	void execute();
	void cancel();
	void delay() { cancel(); }
	std::string name() const { return "sow seeds"; }
	ObjectiveId getObjectiveId() const { return ObjectiveId::SowSeeds; }
	friend class SowSeedsEvent;
	friend class SowSeedsThreadedTask;
};
class SowSeedsEvent final : public ScheduledEventWithPercent
{
	SowSeedsObjective& m_objective;
public:
	SowSeedsEvent(Step delay, SowSeedsObjective& o);
	void execute();
	void clearReferences();
	void onCancel();
};
