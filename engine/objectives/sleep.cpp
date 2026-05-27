#include "sleep.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../path/areaHasPaths.hpp"
#include "../definitions/moveType.h"
// Path Request.
SleepPathRequest::SleepPathRequest(Area&area, SleepObjective &so, const ActorIndex actorIndex) : m_sleepObjective(so)
{
	Actors &actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_sleepObjective.m_detour;
	reserveDestination = true;
}
PathResult SleepPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Actors &actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	assert(actors.sleep_getSpot(actorIndex).empty());
	DesireToSleepAt desireToSleepAtCurrentLocation = m_sleepObjective.desireToSleepAt(area, actors.getLocation(actorIndex), actorIndex);
	if (desireToSleepAtCurrentLocation == DesireToSleepAt::OutsideOrOccupied)
		m_outdoorCandidate = area.getActors().getLocation(actorIndex);
	else if (desireToSleepAtCurrentLocation == DesireToSleepAt::Inside)
		m_indoorCandidate = area.getActors().getLocation(actorIndex);
	else if (desireToSleepAtCurrentLocation == DesireToSleepAt::Designated)
		m_maxDesireCandidate = area.getActors().getLocation(actorIndex);
	if (m_maxDesireCandidate.empty())
	{
		Space& space = area.getSpace();
		constexpr bool useAnyOccupiedPoint = false;
		constexpr bool useAdjacent = false;
		auto shortRangeCondition = [&](const Point3D point, const Facing4 facingForPoint) -> Point3D
		{
			if(!space.shape_shapeAndMoveTypeCanEnterEverWithFacing(point, shape, moveType, facingForPoint))
				return Point3D::null();
			DesireToSleepAt desire = m_sleepObjective.desireToSleepAt(area, point, actorIndex);
			if (desire == DesireToSleepAt::Designated)
			{
				m_maxDesireCandidate = point;
				return point;
			}
			else if (m_indoorCandidate.empty() && desire == DesireToSleepAt::Inside)
				m_indoorCandidate = point;
			else if (m_outdoorCandidate.empty() && desire == DesireToSleepAt::OutsideOrOccupied)
				m_outdoorCandidate = point;
			return Point3D::null();
		};
		// TODO:(optimization) make some filter for long range condition here. Check safe temperature at least.
		auto longRangeCondition = [](const Cuboid) -> bool { return true; };
		return hasPaths.pathToCondition<useAdjacent, useAnyOccupiedPoint>(longRangeCondition, shortRangeCondition, toParamaters(area));
	}
	else
	{
		m_sleepAtCurrentLocation = true;
		return {{}, Point3D::null()};
	}
}
void SleepPathRequest::writeStep(Area&area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if (actors.sleep_getSpot(actorIndex).exists())
	{
		if (useCurrentLocation)
			// Already at sleep spot somehow.
			m_sleepObjective.execute(area, actorIndex);
		else if (path.empty())
		{
			// Cannot path to spot, clear it and search for another place to sleep.
			actors.sleep_setSpot(actorIndex, Point3D::null());
			m_sleepObjective.execute(area, actorIndex);
		}
		else
			// Path found.
			actors.move_setPath(actorIndex, path);
		return;
	}
	// If the current location is the max desired then set sleep at current to true.
	if (m_maxDesireCandidate.exists())
		m_sleepObjective.selectLocation(area, m_maxDesireCandidate, actorIndex);
	else if (m_indoorCandidate.exists())
		m_sleepObjective.selectLocation(area, m_indoorCandidate, actorIndex);
	else if (m_outdoorCandidate.exists())
		m_sleepObjective.selectLocation(area, m_outdoorCandidate, actorIndex);
	else if (m_sleepObjective.m_noWhereToSleepFound)
		// Cannot leave area
		actors.objective_canNotFulfillNeed(actorIndex, m_sleepObjective);
	else
	{
		// No candidates, try to leave area
		m_sleepObjective.m_noWhereToSleepFound = true;
		actors.move_setDestinationToEdge(actorIndex, m_sleepObjective.m_detour);
	}
}
SleepPathRequest::SleepPathRequest(const Json &data, Area&area, DeserializationMemo &deserializationMemo):
	PathRequest(data, area),
	m_sleepObjective(static_cast<SleepObjective &>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
Json SleepPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_sleepObjective);
	output["type"] = "sleep";
	return output;
}
// Sleep Objective.
SleepObjective::SleepObjective() : Objective(Config::sleepObjectivePriority) {}
SleepObjective::SleepObjective(const Json &data, DeserializationMemo &deserializationMemo) : Objective(data, deserializationMemo),
																							 m_noWhereToSleepFound(data["noWhereToSleepFound"].get<bool>()) {}
Json SleepObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereToSleepFound"] = m_noWhereToSleepFound;
	return data;
}
void SleepObjective::execute(Area&area, const ActorIndex actor)
{
	Actors &actors = area.getActors();
	Space &space = area.getSpace();
	assert(actors.sleep_isAwake(actor));
	Point3D sleepingSpot = actors.sleep_getSpot(actor);
	// Check if the spot is still valid.
	if (sleepingSpot.empty())
	{
		if (m_noWhereToSleepFound)
		{
			if (actors.isOnEdge(actor))
				// We are at the edge and can leave.
				actors.leaveArea(actor);
			else
				actors.move_setDestinationToEdge(actor, m_detour);
			return;
		}
		else
			makePathRequest(area, actor);
	}
	else if (actors.getLocation(actor) == sleepingSpot)
	{
		if (desireToSleepAt(area, actors.getLocation(actor), actor) == DesireToSleepAt::Impossible)
		{
			// Can not sleep here any more, look for another spot.
			actors.sleep_setSpot(actor, Point3D::null());
			execute(area, actor);
		}
		else
			// Sleep.
			actors.sleep_do(actor);
	}
	else if (space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(sleepingSpot, actors.getShape(actor), actors.getMoveType(actor)))
	{
		constexpr bool adjacent = false;
		constexpr bool unreserved = true;
		constexpr bool reserve = true;
		actors.move_setDestination(actor, sleepingSpot, m_detour, adjacent, unreserved, reserve);
	}
	else
	{
		// Location no longer can be entered.
		actors.sleep_setSpot(actor, Point3D::null());
		execute(area, actor);
	}
}
DesireToSleepAt SleepObjective::desireToSleepAt(Area& area, const Point3D point, const ActorIndex actor) const
{
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	const MoveTypeId moveType = actors.getMoveType(actor);
	const FactionId& faction = actors.getFaction(actor);
	if (
		(faction.exists() && space.isReserved(point, faction)) ||
		!actors.temperature_isSafe(actor, space.temperature_get(point)) ||
		// Anything which can travel on land or fly will not sleep in fluid.
		// TODO: Make this overridable by species for amphibious animals.
		(space.fluid_any(point) && (MoveType::getSurface(moveType) || MoveType::getFly(moveType)))
	)
		// Impossible to sleep here.
		return DesireToSleepAt::Impossible;
	if (area.m_hasSleepingSpots.containsUnassigned(point))
		// Most desirable.
		return DesireToSleepAt::Designated;
	if (space.isExposedToSky(point) || !space.actor_empty(point))
		// Least deserable.
		return DesireToSleepAt::OutsideOrOccupied;
	else
		// Moderatly desirable.
		return DesireToSleepAt::Inside;
}
void SleepObjective::cancel(Area& area, const ActorIndex actor)
{
	Actors &actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
	actors.sleep_clearObjective(actor);
}
void SleepObjective::reset(Area&area, const ActorIndex actor)
{
	Actors &actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
	m_noWhereToSleepFound = false;
}
void SleepObjective::selectLocation(Area& area, const Point3D location, const ActorIndex actor)
{
	Actors &actors = area.getActors();
	actors.sleep_setSpot(actor, location);
	if (actors.getLocation(actor) != location)
		actors.move_setDestination(actor, location, m_detour);
	else
		execute(area, actor);
}
void SleepObjective::makePathRequest(Area&area, const ActorIndex actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<SleepPathRequest>(area, *this, actor));
}
bool SleepObjective::onCanNotPath(Area&area, const ActorIndex actor)
{
	Actors &actors = area.getActors();
	Point3D sleepSpot = actors.sleep_getSpot(actor);
	if (sleepSpot.exists())
	{
		// Sleep spot exists but we can't get to it. Clear it and look for another.
		actors.sleep_maybeClearSpot(actor);
		execute(area, actor);
		return true;
	}
	return false;
}