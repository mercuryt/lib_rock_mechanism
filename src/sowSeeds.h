#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "path.h"
#include "plant.h"

#include <memory>
#include <vector>

class SowSeedsObjective;

class SowSeedsEvent final : public ScheduledEventWithPercent
{
	SowSeedsObjective& m_objective;
	SowSeedsEvent(uint32_t delay, SowSeedsObjective& o) : ScheduledEventWithPercent(delay), m_objective(o) { }
	void execute();
	~SowSeedsEvent();
};
class SowSeedsThreadedTask final : public ThreadedTask
{
	SowSeedsObjective& m_objective;
	std::vector<Block*> m_result;
	SowSeedsThreadedTask(SowSeedsObjective& sso): m_objective(sso) { }
	void readStep();
	void writeStep();
};
class SowSeedsObjectiveType final : public ObjectiveType
{
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
	SowSeedsObjective(Actor& a) : Objective(Config::sowSeedsPriority), m_actor(a) { }
	void execute();
	void cancel() {}
	friend class SowSeedsEvent;
	friend class SowSeedsThreadedTask;
};
