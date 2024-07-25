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
	bool canBeAssigned(Area& area, ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Harvest; }
	HarvestObjectiveType() = default;
	HarvestObjectiveType(const Json&, DeserializationMemo&);
};
class HarvestObjective final : public Objective
{
	BlockIndex m_block;
public:
	HarvestObjective(Area& area);
	HarvestObjective(const Json& data, Area& area);
	Json toJson() const;
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void select(Area& area, BlockIndex block, ActorIndex actor);
	void begin(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void makePathRequest(Area& area, ActorIndex actor);
	bool canHarvestAt(Area& area, BlockIndex block) const;
	std::string name() const { return "harvest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Harvest; }
	BlockIndex getBlockContainingPlantToHarvestAtLocationAndFacing(Area& area, BlockIndex location, Facing facing, ActorIndex actor);
	bool blockContainsHarvestablePlant(Area& area, BlockIndex block, ActorIndex actor) const;
	friend class HarvestEvent;
	// For testing.
	BlockIndex getBlock() { return m_block; }
};
class HarvestEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(Step delay, Area& area, HarvestObjective& ho, ActorIndex actor, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	Plant* getPlant();
};
class HarvestPathRequest final : public ObjectivePathRequest
{
public:
	HarvestPathRequest(Area& area, HarvestObjective& objective, ActorIndex actor);
	void onSuccess(Area& area, BlockIndex blockWhichPassedPredicate);
};
