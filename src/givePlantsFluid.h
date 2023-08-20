#pragma once

#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "config.h"
#include "eventSchedule.hpp"
#include "pathToBlockBaseThreadedTask.h"

#include <memory>
#include <vector>

class Plant;
class Block;
class Item;
class GivePlantsFluidObjective;
class GivePlantsFluidEvent final : public ScheduledEventWithPercent
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidEvent(Step step, GivePlantsFluidObjective& gpfo) : ScheduledEventWithPercent(step), m_objective(gpfo) { }
	void execute();
	void clearReferences();
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidThreadedTask final : public PathToBlockBaseThreadedTask
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidThreadedTask(GivePlantsFluidObjective& gpfo) : m_objective(gpfo) { }
	void readStep();
	void writeStep();
	void clearReferences();
};
class GivePlantsFluidObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
};
class GivePlantsFluidObjective final : public Objective
{
	Actor& m_actor;
	Plant* m_plant;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
	HasThreadedTask<GivePlantsFluidThreadedTask> m_threadedTask;
public:
	GivePlantsFluidObjective(Actor& a ) : Objective(Config::givePlantsFluidPriority), m_actor(a), m_plant(nullptr) { }
	void execute();
	void cancel() {}
	bool canFillAt(Block& block) const;
	Block* getAdjacentBlockToFillAt(Block& block) const;
	bool canFillItemAt(Block& block) const;
	Item* getItemToFillFromAt(Block& block) const;
	friend class GivePlantsFluidEvent;
	friend class GivePlantsFluidThreadedTask;
};
