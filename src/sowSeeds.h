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
class SowSeedsThreadedTask;
class SowSeedsObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::SowSeeds; }
};
class SowSeedsObjective final : public Objective
{
	Actor& m_actor;
	HasScheduledEvent<SowSeedsEvent> m_event;
	HasThreadedTask<SowSeedsThreadedTask> m_threadedTask;
	Block* m_block;
	Block* getBlockToSowAt(Block& location, Facing facing);
	bool canSowAt(const Block& block) const;
public:
	SowSeedsObjective(Actor& a);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void select(Block& block);
	void begin();
	void reset();
	std::string name() const { return "sow seeds"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::SowSeeds; }
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
};
class SowSeedsThreadedTask final : public ThreadedTask
{
	SowSeedsObjective& m_objective;
	FindsPath m_findsPath;
public:
	SowSeedsThreadedTask(SowSeedsObjective& sso);
	void readStep();
	void writeStep();
	void clearReferences();
};
