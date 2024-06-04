#pragma once

#include "../objective.h"
#include "../config.h"
#include "../eventSchedule.hpp"
#include "../threadedTask.hpp"
#include "../findsPath.h"
#include "../types.h"

struct DeserializationMemo;
class HarvestThreadedTask;
class HarvestEvent;
class Plant;
class Actor;

class HarvestObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Harvest; }
	HarvestObjectiveType() = default;
	HarvestObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class HarvestObjective final : public Objective
{
	BlockIndex m_block = BLOCK_INDEX_MAX;
public:
	HarvestObjective(Actor& a);
	HarvestObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HasThreadedTask<HarvestThreadedTask> m_threadedTask;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void select(BlockIndex block);
	void begin();
	void reset();
	bool canHarvestAt(BlockIndex block) const;
	std::string name() const { return "harvest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Harvest; }
	BlockIndex getBlockContainingPlantToHarvestAtLocationAndFacing(BlockIndex location, Facing facing);
	bool blockContainsHarvestablePlant(BlockIndex block) const;
	friend class HarvestEvent;
	friend class HarvestThreadedTask;
	// For testing.
	BlockIndex getBlock() { return m_block; }
};
class HarvestEvent final : public ScheduledEvent
{
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(Step delay, HarvestObjective& ho, const Step start = 0);
	void execute();
	void clearReferences();
	Plant* getPlant();
};
class HarvestThreadedTask final : public ThreadedTask
{
	HarvestObjective& m_harvestObjective;
	FindsPath m_findsPath;
public:
	HarvestThreadedTask(HarvestObjective& ho);
	void readStep();
	void writeStep();
	void clearReferences();
};
