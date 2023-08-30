#pragma once
#include "objective.h"
#include "plant.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "config.h"

#include <memory>
#include <vector>

class SowSeedsEvent;
class SowSeedsThreadedTask;
class SowSeedsObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
};
class SowSeedsObjective final : public Objective
{
	Actor& m_actor;
	HasScheduledEvent<SowSeedsEvent> m_event;
	HasThreadedTask<SowSeedsThreadedTask> m_threadedTask;
	bool canSowSeedsAt(Block& block);
public:
	SowSeedsObjective(Actor& a);
	void execute();
	void cancel() {}
	std::string name() { return "sow seeds"; }
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
class SowSeedsThreadedTask final : public ThreadedTask
{
	SowSeedsObjective& m_objective;
	std::vector<Block*> m_result;
public:
	SowSeedsThreadedTask(SowSeedsObjective& sso);
	void readStep();
	void writeStep();
	void clearReferences();
};
