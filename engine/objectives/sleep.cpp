#include "sleep.h"
#include "../area.h"
#include "../types.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
// Path Request.
SleepPathRequest::SleepPathRequest(Area& area, SleepObjective& so) : m_sleepObjective(so)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	assert(actors.sleep_getSpot(actor).empty());
	uint32_t desireToSleepAtCurrentLocation = m_sleepObjective.desireToSleepAt(area, actors.getLocation(actor), actor);
	if(desireToSleepAtCurrentLocation == 1)
		m_outdoorCandidate = area.getActors().getLocation(actor);
	else if(desireToSleepAtCurrentLocation == 2)
		m_indoorCandidate = area.getActors().getLocation(actor);
	else if(desireToSleepAtCurrentLocation == 3)
		m_maxDesireCandidate = area.getActors().getLocation(actor);
	if(m_maxDesireCandidate.empty())
	{
		std::function<bool(BlockIndex)> condition = [this, &area, actor](BlockIndex block)
		{
			uint32_t desire = m_sleepObjective.desireToSleepAt(area, block, actor);
			if(desire == 3)
			{
				m_maxDesireCandidate = block;
				return true;
			}
			else if(m_indoorCandidate.empty() && desire == 2)
				m_indoorCandidate = block;	
			else if(m_outdoorCandidate.empty() && desire == 1)
				m_outdoorCandidate = block;	
			return false;
		};
		bool unreserved = actors.getFaction(actor).exists();
		createGoAdjacentToCondition(area, actor, condition, m_sleepObjective.m_detour, unreserved, DistanceInBlocks::null(), BlockIndex::null());
	}
}
void SleepPathRequest::callback(Area& area, FindPathResult&)
{
	ActorIndex actor = getActor();
	Actors& actors = area.getActors();
	// If the current location is the max desired then set sleep at current to true.
	if(m_maxDesireCandidate.exists())
		m_sleepObjective.selectLocation(area, m_maxDesireCandidate, actor);
	else if(m_indoorCandidate.exists())
		m_sleepObjective.selectLocation(area, m_indoorCandidate, actor);
	else if(m_outdoorCandidate.exists())
		m_sleepObjective.selectLocation(area, m_outdoorCandidate, actor);
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
SleepObjective::SleepObjective() : Objective(Config::sleepObjectivePriority) { }
SleepObjective::SleepObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_noWhereToSleepFound(data["noWhereToSleepFound"].get<bool>()) { }
Json SleepObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereToSleepFound"] = m_noWhereToSleepFound;
	return data;
}
void SleepObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	assert(actors.sleep_isAwake(actor));
	BlockIndex sleepingSpot = actors.sleep_getSpot(actor);
	if(sleepingSpot.empty())
	{
		if(m_noWhereToSleepFound)
		{
			if(actors.predicateForAnyOccupiedBlock(actor, [&area](BlockIndex block){ return area.getBlocks().isEdge(block); }))
				// We are at the edge and can leave.
				actors.leaveArea(actor);
			else
				actors.move_setDestinationToEdge(actor, m_detour);
			return;
		}
		else
			makePathRequest(area, actor);
	}
	else if(actors.getLocation(actor) == sleepingSpot)
	{
		if(desireToSleepAt(area, actors.getLocation(actor), actor) == 0)
		{
			// Can not sleep here any more, look for another spot.
			actors.sleep_setSpot(actor, BlockIndex::null());
			execute(area, actor);
		}
		else
			// Sleep.
			actors.sleep_do(actor);
	}
	else
		if(blocks.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(sleepingSpot, actors.getShape(actor), actors.getMoveType(actor)))
			actors.move_setDestination(actor, actors.sleep_getSpot(actor), m_detour);
		else
		{
			// Location no longer can be entered.
			actors.sleep_setSpot(actor, BlockIndex::null());
			execute(area, actor);
		}
}
uint32_t SleepObjective::desireToSleepAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	if(blocks.isReserved(block, actors.getFactionId(actor)) || !actors.temperature_isSafe(actor, blocks.temperature_get(block)))
		return 0;
	if(area.m_hasSleepingSpots.containsUnassigned(block))
		return 3;
	if(blocks.isOutdoors(block))
		return 1;
	else
		return 2;
}
void SleepObjective::cancel(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
void SleepObjective::reset(Area& area, ActorIndex actor)
{
	cancel(area, actor);
	m_noWhereToSleepFound = false;
}
void SleepObjective::selectLocation(Area& area, BlockIndex location, ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.sleep_setSpot(actor, location);
	actors.move_setDestination(actor, location, m_detour);
}
void SleepObjective::makePathRequest(Area& area, ActorIndex actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<SleepPathRequest>(area, *this));
}
bool SleepObjective::onCanNotRepath(Area& area, ActorIndex actor)
{
	BlockIndex sleepSpot = area.getActors().sleep_getSpot(actor);
	if(sleepSpot.empty())
		return false;
	sleepSpot.clear();
	execute(area, actor);
	return true;
}
