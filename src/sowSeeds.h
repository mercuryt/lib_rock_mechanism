#pragma once
#include "eventSchedule.h"
#include "threadedTask.h"
#include "objective.h"
#include "path.h"
#include "plant.h"

#include <memory>
#include <vector>

class SowSeedsEvent : ScheduledEventWithPercent
{
	SowSeedsObjective m_sowSeedsObjective;
	void execute()
	{
		Block& location = *m_sowSeedsObjective.m_actor.m_location;
		const PlantType& plantType = location.m_area->m_hasFarmFields.getPlantTypeFor(location);
		location.m_containsPlants.add(plantType);
		m_sowSeedsObjective.m_actor.m_hasObjectives.objectiveComplete(m_sowSeedsObjective);
		location.m_area->m_hasFarmFields.removeAllSowSeedsDesignations(location);
	}
	~SowSeedsObjective(){ m_sowSeedsObjective.m_event.clearPointer(); }
};
class SowSeedsThreadedTask : ThreadedTask
{
	SowSeedsObjective& m_objective;
	std::vector<Block*> m_result;
	SowSeedsThreadedTask(SowSeedsObjective& sso): m_objective(sso) { }
	void readStep()
	{
		auto condition = [&](Block* block)
		{
			return block->m_hasDesignations.contains(m_objective.m_actor.getPlayer(), BlockDesignation::SowSeeds);
		};
		m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
	}
	void writeStep()
	{
		if(m_result.empty())
			m_objective.m_actor.m_hasObjectives.cannotCompleteObjective(m_objective);
		else
			if(m_result.back()->m_reservable.isFullyReserved())
				m_objective.m_threadedTask.create(m_objective);
			else
			{
				m_result.back().m_reservable.reserveFor(m_objective.m_actor.m_canReserve);
				m_actor.setPath(m_result);
			}
	}
};
class SowSeedsObjectiveType : ObjectiveType
{
	bool canBeAssigned(Actor& actor)
	{
		return actor.m_location->m_area->m_hasFarmFields.hasSowSeedDesignations();
	}
	std::unique_ptr<Objective> makeFor(Actor& actor)
	{
		return std::make_unique<SowSeedsObjective>(actor);
	}
};
class SowSeedsObjective : Objective
{
	Actor& m_actor;
	HasScheduledEvent<SowSeedsEvent> m_event;
	HasThreadedTask<SowSeedsThreadedTask> m_threadedTask;
	SowSeedsObjective(Actor& a) : Objective(Config::sowSeedsPriority), m_actor(a) { }
	void execute()
	{
		if(canSowSeedsAt(*m_actor.m_location))
			m_event.schedule(Config::SowSeedsStepsDuration, *this);
		else
			m_threadedTask.create(*this);
	}
};
