#include "sowSeeds.h"
#include "area.h"
#include "actor.h"
#include "farmFields.h"
#include "simulation.h"

struct DeserializationMemo;

SowSeedsEvent::SowSeedsEvent(Step delay, SowSeedsObjective& o, const Step start) : ScheduledEvent(o.m_actor.getSimulation(), delay, start), m_objective(o) { }
void SowSeedsEvent::execute()
{
	Block& block = *m_objective.m_block;
	const Faction& faction = *m_objective.m_actor.getFaction();
	if(!block.m_isPartOfFarmField.contains(faction))
	{
		// Block is no longer part of a field. It may have been undesignated or it may no longer be a suitable place to grow the selected plant.
		m_objective.reset();
		m_objective.execute();
	}
	const PlantSpecies& plantSpecies = block.m_area->m_hasFarmFields.getPlantSpeciesFor(faction, block);
	block.m_hasPlant.createPlant(plantSpecies);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
void SowSeedsEvent::clearReferences(){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasFarmFields.hasSowSeedsDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<SowSeedsObjective>(actor);
}
SowSeedsThreadedTask::SowSeedsThreadedTask(SowSeedsObjective& sso): ThreadedTask(sso.m_actor.getThreadedTaskEngine()), m_objective(sso), m_findsPath(sso.m_actor, sso.m_detour) { }
void SowSeedsThreadedTask::readStep()
{
	const Faction* faction = m_objective.m_actor.getFaction();
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return m_objective.canSowAt(block); };
	m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *faction);
}
void SowSeedsThreadedTask::writeStep()
{
	assert(m_objective.m_block == nullptr);
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation)
		{
			m_objective.select(*m_findsPath.getBlockWhichPassedPredicate());
			m_objective.execute();
		}
		else
			// Cannot find any accessable field to sow.
			m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	}
	else
	{
		if(!m_findsPath.areAllBlocksAtDestinationReservable(m_objective.m_actor.getFaction()))
			// Cannot reserve location, try again.
			m_objective.m_threadedTask.create(m_objective);
		else
		{
			Block* block = m_findsPath.getBlockWhichPassedPredicate();
			if(!m_objective.canSowAt(*block))
				// Selected destination is no longer adjacent to a block where we can sow, try again.
				m_objective.m_threadedTask.create(m_objective);
			else
			{
				// Found a field to sow.
				m_objective.select(*block);
				m_findsPath.reserveBlocksAtDestination(m_objective.m_actor.m_canReserve);
				m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath());
			}
		}
	}
}
void SowSeedsThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
SowSeedsObjective::SowSeedsObjective(Actor& a) : Objective(a, Config::sowSeedsPriority), m_event(a.getEventSchedule()), m_threadedTask(a.getThreadedTaskEngine()), m_block(nullptr) { }
SowSeedsObjective::SowSeedsObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_event(deserializationMemo.m_simulation.m_eventSchedule), m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_block(data.contains("block") ? &deserializationMemo.m_simulation.getBlockForJsonQuery(data["block"]) : nullptr)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_event.schedule(Config::sowSeedsStepsDuration, *this, data["eventStart"].get<Step>());
}
Json SowSeedsObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_block)
		data["block"] = m_block;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
Block* SowSeedsObjective::getBlockToSowAt(Block& location, Facing facing)
{
	const Faction* faction = m_actor.getFaction();
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return block.m_hasDesignations.contains(*faction, BlockDesignation::SowSeeds) && !block.m_reservable.isFullyReserved(faction);
	};
	return m_actor.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
}
void SowSeedsObjective::execute()
{
	if(m_block != nullptr)
	{
		if(m_actor.isAdjacentTo(*m_block))
		{
			FarmField* field = m_block->m_isPartOfFarmField.get(*m_actor.getFaction());
			if(field != nullptr && m_block->m_hasPlant.canGrowHereAtSomePointToday(*field->plantSpecies))
			{
				begin();
				return;
			}
		}
		// Previously found m_block or path no longer valid, redo from start.
		reset();
		execute();
	}
	else
	{
		// Check if we can use an adjacent as m_block.
		if(m_actor.allOccupiedBlocksAreReservable(*m_actor.getFaction()))
		{
			Block* block = getBlockToSowAt(*m_actor.m_location, m_actor.m_facing);
			if(block != nullptr)
			{
				select(*block);
				m_actor.reserveOccupied(m_actor.m_canReserve);
				begin();
				return;
			}
		}
		// Try to find m_block and path.
		m_threadedTask.create(*this);
	}
}
void SowSeedsObjective::cancel()
{
	m_actor.m_canReserve.deleteAllWithoutCallback();
	m_threadedTask.maybeCancel();
	m_event.maybeUnschedule();
	if(m_block != nullptr && m_block->m_isPartOfFarmField.contains(*m_actor.getFaction()))
	{
		FarmField* field = m_block->m_isPartOfFarmField.get(*m_actor.getFaction());
		if(field == nullptr)
			return;
		//TODO: check it is still planting season.
		if(m_block->m_hasPlant.canGrowHereAtSomePointToday(*field->plantSpecies))
			m_block->m_area->m_hasFarmFields.at(*m_actor.getFaction()).addSowSeedsDesignation(*m_block);
	}
}
void SowSeedsObjective::select(Block& block)
{
	assert(!block.m_hasPlant.exists());
	assert(block.m_isPartOfFarmField.contains(*m_actor.getFaction()));
	assert(m_block == nullptr);
	m_block = &block;
	block.m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeSowSeedsDesignation(block);
	block.m_reservable.reserveFor(m_actor.m_canReserve, 1u);
}
void SowSeedsObjective::begin()
{
	assert(m_block != nullptr);
	assert(m_actor.isAdjacentTo(*m_block));
	m_event.schedule(Config::sowSeedsStepsDuration, *this);
}
void SowSeedsObjective::reset()
{
	cancel();
	m_block = nullptr;
}
bool SowSeedsObjective::canSowAt(const Block& block) const
{
	const Faction* faction = m_actor.getFaction();
	return block.m_hasDesignations.contains(*faction, BlockDesignation::SowSeeds) && !block.m_reservable.isFullyReserved(faction);
}
