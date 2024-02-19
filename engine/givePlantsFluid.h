#pragma once

#include "deserializationMemo.h"
#include "objective.h"
#include "eventSchedule.hpp"
#include "reservable.h"
#include "threadedTask.hpp"
#include "findsPath.h"

#include <memory>
#include <vector>

struct FluidType;
class Plant;
class Block;
class Item;
class GivePlantsFluidObjective;
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
	Block* m_plantLocation;
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
class GivePlantsFluidItemDishonorCallback final : public DishonorCallback
{
	Actor& m_actor;
public:
	GivePlantsFluidItemDishonorCallback(Actor& a) : m_actor(a) { }
	GivePlantsFluidItemDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(uint32_t oldCount, uint32_t newCount);
};
class GivePlantsFluidObjective final : public Objective
{
	Block* m_plantLocation;
	Item* m_fluidHaulingItem;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
	HasThreadedTask<GivePlantsFluidThreadedTask> m_threadedTask;
public:
	GivePlantsFluidObjective(Actor& a);
	GivePlantsFluidObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void fillContainer(Block& fillLocation);
	void delay() { cancel(); }
	void select(Block& block);
	void select(Item& item);
	void reset();
	std::string name() const { return "give plants fluid"; }
	bool canFillAt(const Block& block) const;
	Item* getItemToFillFromAt(Block& block);
	bool canGetFluidHaulingItemAt(const Block& location) const;
	Item* getFluidHaulingItemAt(Block& location);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GivePlantsFluid; }
	friend class GivePlantsFluidEvent;
	friend class GivePlantsFluidThreadedTask;
	// For testing.
	Block* getPlantLocation() { return m_plantLocation; }
};
