#pragma once

#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "path.h"
#include "plant.h"

#include <memory>
#include <vector>

class GivePlantsFluidEvent : ScheduledEventWithPercent
{
	GivePlantsFluidObjective& m_objective;
	GivePlantsFluidEvent(uint32_t step, GivePlantsFluidObjective& gpfo) : ScheduledEvent(step), m_objective(gpfo) { }
	void execute();
	~GivePlantsFluidEvent();
};
// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidThreadedTask : ThreadedTask
{
	GivePlantsFluidObjective& m_objective;
	std::vector<Block*> m_result;
	GivePlantsFluidThreadedTask(GivePlantsFluidObjective* gpfo) : m_objective(gpfo) { }
	void readStep();
	void writeStep();
};
class GivePlantsFluidObjectiveType : ObjectiveType
{
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<Objective> makeFor(Actor& actor);
};
class GivePlantsFluidObjective : Objective
{
	Actor& m_actor;
	Plant* m_plant;
	HasScheduledEvent<GivePlantsFluidEvent> m_event;
	HasThreadedTask<GivePlantsFluidThreadedTask> m_threadedTask;
	GivePlantsFluidObjective(Actor& a ) : m_actor(a), m_plant(nullptr) { }
	void execute();
	bool canFillAt(Block& block) const;
	Block* getAdjacentBlockToFillAt(Block& block) const;
	bool canFillItemAt(Block* block) const;
	Item* getItemToFillFromAt(Block* block) const;
};
