#pragma once

#include "objective.h"
#include "eventSchedule.hpp"
#include "threadedTask.hpp"
#include "findsPath.h"

#include <memory>
#include <vector>

struct FluidType;
class Plant;
class Block;
class Item;
class GivePlantsFluidObjective;
class GivePlantsFluidEvent final : public ScheduledEventWithPercent
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidEvent(Step step, GivePlantsFluidObjective& gpfo);
	void execute();
	void clearReferences();
	void onCancel();
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidThreadedTask final : public ThreadedTask
{
	GivePlantsFluidObjective& m_objective;
	Item* m_haulTool;
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
	ObjectiveId getObjectiveId() const { return ObjectiveId::GivePlantsFluid; }
};
class GivePlantsFluidObjective final : public Objective
{
	Actor& m_actor;
	Plant* m_plant;
	Item* m_haulTool;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
	HasThreadedTask<GivePlantsFluidThreadedTask> m_threadedTask;
public:
	GivePlantsFluidObjective(Actor& a );
	void execute();
	void cancel();
	void delay() { cancel(); }
	std::string name() const { return "give plants fluid"; }
	bool canFillAt(const Block& block) const;
	Block* getAdjacentBlockToFillAt(Block& block);
	bool canFillFromItemAt(const Block& block) const;
	Item* getItemToFillFromAt(Block& block);
	bool canGetFluidHaulingItemAt(const Block& block) const;
	Item* getFluidHaulingItemAt(Block& block);
	ObjectiveId getObjectiveId() const { return ObjectiveId::GivePlantsFluid; }
	friend class GivePlantsFluidEvent;
	friend class GivePlantsFluidThreadedTask;
};
