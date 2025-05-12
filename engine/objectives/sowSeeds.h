#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../path/terrainFacade.h"
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
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	SowSeedsObjectiveType() = default;
	SowSeedsObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "sow seeds"; }
};
class SowSeedsObjective final : public Objective
{
	HasScheduledEvent<SowSeedsEvent> m_event;
	BlockIndex m_block;
public:
	SowSeedsObjective(Area& area);
	SowSeedsObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void select(Area& area, const BlockIndex& block, const ActorIndex& actor);
	void begin(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "sow seeds"; }
	[[nodiscard]] bool canSowAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	[[nodiscard]] BlockIndex getBlockToSowAt(Area& area, const BlockIndex& location, Facing4 facing, const ActorIndex& actor);
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class SowSeedsEvent;
	// For testing.
	[[nodiscard]] BlockIndex getBlock() { return m_block; }
};
class SowSeedsEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	SowSeedsObjective& m_objective;
public:
	SowSeedsEvent(const Step& delay, Area& area, SowSeedsObjective& o, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class SowSeedsPathRequest final : public PathRequestBreadthFirst
{
	SowSeedsObjective& m_objective;
public:
	SowSeedsPathRequest(Area& area, SowSeedsObjective& objective, const ActorIndex& actor);
	SowSeedsPathRequest(const Json& data, Area& area, DeserializationMemo& DeserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() const { return "sow seeds"; }
	[[nodiscard]] Json toJson() const;
};
