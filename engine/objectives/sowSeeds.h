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
	SowSeedsObjectiveType() = default;
	SowSeedsObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "sow"; }
};
class SowSeedsObjective final : public Objective
{
	HasScheduledEvent<SowSeedsEvent> m_event;
	BlockIndex m_block;
public:
	SowSeedsObjective(Area& area);
	SowSeedsObjective(const Json& data, Area& area, ActorIndex actor);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void select(Area& area, BlockIndex block, ActorIndex actor);
	void begin(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "sow seeds"; }
	[[nodiscard]] bool canSowAt(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] BlockIndex getBlockToSowAt(Area& area, BlockIndex location, Facing facing, ActorIndex actor);
	friend class SowSeedsEvent;
	// For testing.
	[[nodiscard]] BlockIndex getBlock() { return m_block; }
};
class SowSeedsEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	SowSeedsObjective& m_objective;
public:
	SowSeedsEvent(Step delay, Area& area, SowSeedsObjective& o, ActorIndex actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class SowSeedsPathRequest final : public ObjectivePathRequest
{
public:
	SowSeedsPathRequest(Area& area, SowSeedsObjective& objective);
	void onSuccess(Area& area, BlockIndex blockWhichPassedPredicate);
};
