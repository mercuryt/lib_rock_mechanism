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
	BlockIndex m_block = BLOCK_INDEX_MAX;
public:
	HarvestObjective(Area& area, ActorIndex a);
	HarvestObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void select(Area& area, BlockIndex block);
	void begin(Area& area);
	void reset(Area& area);
	void makePathRequest(Area& area);
	bool canHarvestAt(Area& area, BlockIndex block) const;
	std::string name() const { return "harvest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Harvest; }
	BlockIndex getBlockContainingPlantToHarvestAtLocationAndFacing(Area& area, BlockIndex location, Facing facing);
	bool blockContainsHarvestablePlant(Area& area, BlockIndex block) const;
	friend class HarvestEvent;
	// For testing.
	BlockIndex getBlock() { return m_block; }
};
class HarvestEvent final : public ScheduledEvent
{
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(Step delay, Area& area, HarvestObjective& ho, const Step start = 0);
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	Plant* getPlant();
};
class HarvestPathRequest final : public ObjectivePathRequest
{
public:
	HarvestPathRequest(Area& area, HarvestObjective& objective);
	void onSuccess(Area& area, BlockIndex blockWhichPassedPredicate);
};
