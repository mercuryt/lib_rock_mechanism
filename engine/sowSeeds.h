#pragma once
#include "deserializationMemo.h"
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
	SowSeedsObjectiveType() = default;
	SowSeedsObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class SowSeedsObjective final : public Objective
{
	HasScheduledEvent<SowSeedsEvent> m_event;
	HasThreadedTask<SowSeedsThreadedTask> m_threadedTask;
	Block* m_block;
	Block* getBlockToSowAt(Block& location, Facing facing);
	bool canSowAt(const Block& block) const;
public:
	SowSeedsObjective(Actor& a);
	SowSeedsObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
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
	// For testing.
	Block* getBlock() { return m_block; }
};
class SowSeedsEvent final : public ScheduledEvent
{
	SowSeedsObjective& m_objective;
public:
	SowSeedsEvent(Step delay, SowSeedsObjective& o, const Step start = 0);
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
