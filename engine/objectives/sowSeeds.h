#pragma once
#include "objective.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "config.h"
#include "findsPath.h"
#include "types.h"

#include <memory>
#include <vector>

class SowSeedsEvent;
class SowSeedsThreadedTask;
struct DeserializationMemo;
class SowSeedsObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Actor& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Actor& actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::SowSeeds; }
	SowSeedsObjectiveType() = default;
	SowSeedsObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class SowSeedsObjective final : public Objective
{
	HasScheduledEvent<SowSeedsEvent> m_event;
	HasThreadedTask<SowSeedsThreadedTask> m_threadedTask;
	BlockIndex m_block = BLOCK_INDEX_MAX;
public:
	SowSeedsObjective(Actor& a);
	SowSeedsObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void select(BlockIndex block);
	void begin();
	void reset();
	[[nodiscard]] std::string name() const { return "sow seeds"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::SowSeeds; }
	[[nodiscard]] bool canSowAt(BlockIndex block) const;
	[[nodiscard]] BlockIndex getBlockToSowAt(BlockIndex location, Facing facing);
	friend class SowSeedsEvent;
	friend class SowSeedsThreadedTask;
	// For testing.
	[[nodiscard]] BlockIndex getBlock() { return m_block; }
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
