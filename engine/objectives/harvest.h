#pragma once

#include "../objective.h"
#include "../config.h"
#include "../eventSchedule.hpp"
#include "../pathRequest.h"
#include "../types.h"

struct DeserializationMemo;
class HarvestEvent;

class HarvestObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	HarvestObjectiveType() = default;
	HarvestObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "harvest"; }
};
class HarvestObjective final : public Objective
{
	BlockIndex m_block;
public:
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HarvestObjective(Area& area);
	HarvestObjective(const Json& data, Area& area);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void select(Area& area, BlockIndex block, ActorIndex actor);
	void begin(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void makePathRequest(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canHarvestAt(Area& area, BlockIndex block) const;
	[[nodiscard]] std::string name() const { return "harvest"; }
	[[nodiscard]] BlockIndex getBlockContainingPlantToHarvestAtLocationAndFacing(Area& area, BlockIndex location, Facing facing, ActorIndex actor);
	[[nodiscard]] bool blockContainsHarvestablePlant(Area& area, BlockIndex block, ActorIndex actor) const;
	friend class HarvestEvent;
	// For testing.
	BlockIndex getBlock() { return m_block; }
};
class HarvestEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(Step delay, Area& area, HarvestObjective& ho, ActorIndex actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	PlantIndex getPlant();
};
class HarvestPathRequest final : public ObjectivePathRequest
{
public:
	HarvestPathRequest(Area& area, HarvestObjective& objective, ActorIndex actor);
	HarvestPathRequest(const Json& data, DeserializationMemo& deserializationMemo) : ObjectivePathRequest(data, deserializationMemo) { }
	void onSuccess(Area& area, BlockIndex blockWhichPassedPredicate);
	[[nodiscard]] std::string name() { return "harvest"; }
};
