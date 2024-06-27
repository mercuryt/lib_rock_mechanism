#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../terrainFacade.h"
#include "../eventSchedule.hpp"
#include "../config.h"
#include "../types.h"

#include <memory>
#include <vector>

class SowSeedsEvent;
class SowSeedsPathRequest;
struct DeserializationMemo;
class SowSeedsObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::SowSeeds; }
	SowSeedsObjectiveType() = default;
	SowSeedsObjectiveType(const Json&, DeserializationMemo&);
};
class SowSeedsObjective final : public Objective
{
	HasScheduledEvent<SowSeedsEvent> m_event;
	BlockIndex m_block = BLOCK_INDEX_MAX;
public:
	SowSeedsObjective(Area& area, ActorIndex a);
	SowSeedsObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void select(Area& area, BlockIndex block);
	void begin(Area& area);
	void reset(Area& area);
	[[nodiscard]] std::string name() const { return "sow seeds"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::SowSeeds; }
	[[nodiscard]] bool canSowAt(Area& area, BlockIndex block) const;
	[[nodiscard]] BlockIndex getBlockToSowAt(Area& area, BlockIndex location, Facing facing);
	friend class SowSeedsEvent;
	// For testing.
	[[nodiscard]] BlockIndex getBlock() { return m_block; }
};
class SowSeedsEvent final : public ScheduledEvent
{
	SowSeedsObjective& m_objective;
public:
	SowSeedsEvent(Area& area, Step delay, SowSeedsObjective& o, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class SowSeedsPathRequest final : public ObjectivePathRequest
{
public:
	SowSeedsPathRequest(Area& area, SowSeedsObjective& objective);
	void onSuccess(Area& area, BlockIndex blockWhichPassedPredicate);
};
