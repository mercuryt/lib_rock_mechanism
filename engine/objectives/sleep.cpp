#include "sleep.h"
#include "../area.h"
#include "../types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
// Path Request.
SleepPathRequest::SleepPathRequest(Area& area, SleepObjective& so) : m_sleepObjective(so)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_sleepObjective.m_actor;
	assert(actors.sleep_getSpot(m_sleepObjective.m_actor) == BLOCK_INDEX_MAX);
	uint32_t desireToSleepAtCurrentLocation = m_sleepObjective.desireToSleepAt(area, actors.getLocation(actor));
	if(desireToSleepAtCurrentLocation == 1)
		m_outdoorCandidate = area.getActors().getLocation(m_sleepObjective.m_actor);
	else if(desireToSleepAtCurrentLocation == 2)
		m_indoorCandidate = area.getActors().getLocation(m_sleepObjective.m_actor);
	else if(desireToSleepAtCurrentLocation == 3)
		m_maxDesireCandidate = area.getActors().getLocation(m_sleepObjective.m_actor);
	if(m_maxDesireCandidate == BLOCK_INDEX_MAX)
	{
		std::function<bool(BlockIndex)> condition = [this, &area](BlockIndex block)
		{
			uint32_t desire = m_sleepObjective.desireToSleepAt(area, block);
			if(desire == 3)
			{
				m_maxDesireCandidate = block;
				return true;
			}
			else if(m_indoorCandidate == BLOCK_INDEX_MAX && desire == 2)
				m_indoorCandidate = block;	
			else if(m_outdoorCandidate == BLOCK_INDEX_MAX && desire == 1)
				m_outdoorCandidate = block;	
			return false;
		};
		bool unreserved = actors.getFaction(actor) != nullptr;
		createGoAdjacentToCondition(area, actor, condition, m_sleepObjective.m_detour, unreserved, BLOCK_DISTANCE_MAX, BLOCK_INDEX_MAX);
	}
}
void SleepPathRequest::callback(Area& area, FindPathResult)
{
	ActorIndex actor = m_sleepObjective.m_actor;
	Actors& actors = area.getActors();
	// If the current location is the max desired then set sleep at current to true.
	if(m_maxDesireCandidate != BLOCK_INDEX_MAX)
		m_sleepObjective.selectLocation(area, m_maxDesireCandidate);
	else if(m_indoorCandidate != BLOCK_INDEX_MAX)
		m_sleepObjective.selectLocation(area, m_indoorCandidate);
	else if(m_outdoorCandidate != BLOCK_INDEX_MAX)
		m_sleepObjective.selectLocation(area, m_outdoorCandidate);
	else if(m_sleepObjective.m_noWhereToSleepFound)
		// Cannot leave area
		actors.objective_canNotFulfillNeed(actor, m_sleepObjective);
	else
	{
		// No candidates, try to leave area
		m_sleepObjective.m_noWhereToSleepFound = true;
		createGoToEdge(area, actor, m_sleepObjective.m_detour);
	}
}
// Sleep Objective.
SleepObjective::SleepObjective(ActorIndex a) : Objective(a, Config::sleepObjectivePriority) { }
/*
SleepObjective::SleepObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo),
	m_pathRequest(deserializationMemo.m_simulation.m_pathRequestEngine), m_noWhereToSleepFound(data["noWhereToSleepFound"].get<bool>())
{
	if(data.contains("pathRequest"))
		m_pathRequest.create(*this);
}
Json SleepObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereToSleepFound"] = m_noWhereToSleepFound;
	if(m_pathRequest.exists())
		data["pathRequest"] = true;
	return data;
}
*/
void SleepObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	assert(actors.sleep_isAwake(m_actor));
	BlockIndex sleepingSpot = actors.sleep_getSpot(m_actor);
	if(sleepingSpot == BLOCK_INDEX_MAX)
	{
		if(m_noWhereToSleepFound)
		{
			if(actors.predicateForAnyOccupiedBlock(m_actor, [&area](BlockIndex block){ return area.getBlocks().isEdge(block); }))
				// We are at the edge and can leave.
				actors.leaveArea(m_actor);
			else
				actors.move_setDestinationToEdge(m_actor, m_detour);
			return;
		}
		else
			makePathRequest(area);
	}
	else if(actors.getLocation(m_actor) == sleepingSpot)
	{
		if(desireToSleepAt(area, actors.getLocation(m_actor)) == 0)
		{
			// Can not sleep here any more, look for another spot.
			actors.sleep_setSpot(m_actor, BLOCK_INDEX_MAX);
			execute(area);
		}
		else
			// Sleep.
			actors.sleep_do(m_actor);
	}
	else
		if(blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(sleepingSpot, actors.getShape(m_actor), actors.getMoveType(m_actor)))
			actors.move_setDestination(m_actor, actors.sleep_getSpot(m_actor), m_detour);
		else
		{
			// Location no longer can be entered.
			actors.sleep_setSpot(m_actor, BLOCK_INDEX_MAX);
			execute(area);
		}
}
uint32_t SleepObjective::desireToSleepAt(Area& area, BlockIndex block) const
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	if(blocks.isReserved(block, *actors.getFaction(m_actor)) || !actors.temperature_isSafe(m_actor, blocks.temperature_get(block)))
		return 0;
	if(area.m_hasSleepingSpots.containsUnassigned(block))
		return 3;
	if(blocks.isOutdoors(block))
		return 1;
	else
		return 2;
}
void SleepObjective::cancel(Area& area)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.canReserve_clearAll(m_actor);
}
void SleepObjective::reset(Area& area)
{
	cancel(area);
	m_noWhereToSleepFound = false;
}
void SleepObjective::selectLocation(Area& area, BlockIndex location)
{
	Actors& actors = area.getActors();
	actors.sleep_setSpot(m_actor, location);
	actors.move_setDestination(m_actor, location, m_detour);
}
bool SleepObjective::onCanNotRepath(Area& area)
{
	BlockIndex sleepSpot = area.getActors().sleep_getSpot(m_actor);
	if(sleepSpot == BLOCK_INDEX_MAX)
		return false;
	sleepSpot = BLOCK_INDEX_MAX;
	execute(area);
	return true;
}
//SleepObjective::~SleepObjective() { m_actor.m_mustSleep.m_objective = nullptr; }
