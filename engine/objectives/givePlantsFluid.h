#pragma once

#include "objective.h"
#include "eventSchedule.hpp"
#include "reservable.h"
#include "threadedTask.hpp"
#include "findsPath.h"
#include "types.h"

#include <memory>
#include <vector>

struct FluidType;
class Plant;
class Item;
class GivePlantsFluidObjective;
struct DeserializationMemo;

class GivePlantsFluidEvent final : public ScheduledEvent
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidEvent(Step step, GivePlantsFluidObjective& gpfo, const Step start = 0);
	void execute();
	void clearReferences();
	void onCancel();
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidThreadedTask final : public ThreadedTask
{
	GivePlantsFluidObjective& m_objective;
	BlockIndex m_plantLocation = BLOCK_INDEX_MAX;
	FindsPath m_findsPath;
public:
	GivePlantsFluidThreadedTask(GivePlantsFluidObjective& gpfo);
	void readStep();
	void writeStep();
	void clearReferences();
};
class GivePlantsFluidObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GivePlantsFluid; }
	GivePlantsFluidObjectiveType() = default;
	GivePlantsFluidObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class GivePlantsFluidObjective final : public Objective
{
	BlockIndex m_plantLocation = BLOCK_INDEX_MAX;
	Item* m_fluidHaulingItem = nullptr;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
	HasThreadedTask<GivePlantsFluidThreadedTask> m_threadedTask;
public:
	GivePlantsFluidObjective(Actor& a);
	GivePlantsFluidObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void fillContainer(BlockIndex fillLocation);
	void delay() { cancel(); }
	void select(BlockIndex block);
	void select(Item& item);
	void reset();
	std::string name() const { return "give plants fluid"; }
	bool canFillAt(BlockIndex block) const;
	Item* getItemToFillFromAt(BlockIndex block);
	bool canGetFluidHaulingItemAt(BlockIndex location) const;
	Item* getFluidHaulingItemAt(BlockIndex location);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GivePlantsFluid; }
	friend class GivePlantsFluidEvent;
	friend class GivePlantsFluidThreadedTask;
	// For testing.
	BlockIndex getPlantLocation() { return m_plantLocation; }
};
