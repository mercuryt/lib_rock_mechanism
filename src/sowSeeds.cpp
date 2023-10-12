#include "sowSeeds.h"
#include "area.h"
#include "actor.h"
SowSeedsEvent::SowSeedsEvent(Step delay, SowSeedsObjective& o) : ScheduledEventWithPercent(o.m_actor.getSimulation(), delay), m_objective(o) { }
void SowSeedsEvent::execute()
{
	Block& location = *m_objective.m_actor.m_location;
	const PlantSpecies& plantSpecies = location.m_area->m_hasFarmFields.getPlantSpeciesFor(*m_objective.m_actor.getFaction(), location);
	location.m_hasPlant.addPlant(plantSpecies);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
void SowSeedsEvent::onCancel()
{
	Block& location = *m_objective.m_actor.m_location;
	location.m_area->m_hasFarmFields.at(*m_objective.m_actor.getFaction()).addSowSeedsDesignation(location);
}
void SowSeedsEvent::clearReferences(){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasSowSeedsDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Actor& actor) const { return std::make_unique<SowSeedsObjective>(actor); }
SowSeedsObjective::SowSeedsObjective(Actor& a) : Objective(Config::sowSeedsPriority), m_actor(a), m_event(a.getEventSchedule()) { }
void SowSeedsObjective::execute()
{
	const Faction* faction = m_actor.getFaction();
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{ 
		return block.m_hasDesignations.contains(*faction, BlockDesignation::SowSeeds) && !block.m_reservable.isFullyReserved(faction);
	};
	std::function<void(Block&)> callback = [&](Block& block)
	{
		block.m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeSowSeedsDesignation(block);
		m_event.schedule(Config::sowSeedsStepsDuration, *this);
	};
	// predicate, detour, adjacent, unreserved.
	m_actor.m_canMove.goToPredicateBlockAndThen(predicate, callback, false, true, true);
}
void SowSeedsObjective::cancel() { m_event.maybeUnschedule(); }
