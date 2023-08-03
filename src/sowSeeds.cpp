#include "sowSeeds.h"
#include "area.h"
void SowSeedsEvent::execute()
{
	Block& location = *m_objective.m_actor.m_location;
	const PlantSpecies& plantSpecies = location.m_area->m_hasFarmFields.getPlantSpeciesFor(*m_objective.m_actor.m_faction, location);
	location.m_hasPlant.addPlant(plantSpecies);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
	location.m_area->m_hasFarmFields.removeAllSowSeedsDesignations(location);
}
SowSeedsEvent::~SowSeedsEvent(){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasSowSeedsDesignations(*actor.m_faction);
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<SowSeedsObjective>(actor);
}
void SowSeedsThreadedTask::readStep()
{
	auto condition = [&](Block& block)
	{
		return block.m_hasDesignations.contains(*m_objective.m_actor.m_faction, BlockDesignation::SowSeeds);
	};
	m_result = path::getForActorToPredicate(m_objective.m_actor, condition);
}
void SowSeedsThreadedTask::writeStep()
{
	if(m_result.empty())
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
		if(m_result.back()->m_reservable.isFullyReserved(*m_objective.m_actor.m_faction))
			m_objective.m_threadedTask.create(m_objective);
		else
		{
			m_result.back()->m_reservable.reserveFor(m_objective.m_actor.m_canReserve, 1);
			m_objective.m_actor.m_canMove.setPath(m_result);
		}
}
bool SowSeedsObjective::canSowSeedsAt(Block& block)
{
	return block.m_hasDesignations.contains(*m_actor.m_faction, BlockDesignation::SowSeeds);
}
void SowSeedsObjective::execute()
{
	if(canSowSeedsAt(*m_actor.m_location))
		m_event.schedule(Config::sowSeedsStepsDuration, *this);
	else
		m_threadedTask.create(*this);
}
