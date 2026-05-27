#include "wander.h"
#include "../area/area.h"
#include "../deserializationMemo.h"
#include "../random.h"
#include "../config/config.h"
#include "../simulation/simulation.h"
#include "../path/areaHasPaths.hpp"
#include "../actors/actors.h"
#include "../numericTypes/types.h"
// PathRequest
WanderPathRequest::WanderPathRequest(Area& area, WanderObjective& objective, const ActorIndex actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	facing = actors.getFacing(actorIndex);
	maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
}
WanderPathRequest::WanderPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_objective(static_cast<WanderObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))),
	m_lastPoint(data["lastPoint"].get<Point3D>())
{ }
PathResult WanderPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	Random& random = area.m_simulation.m_random;
	m_pointCounter = random.getInRange(Config::wanderMinimimNumberOfPoints, Config::wanderMaximumNumberOfPoints);
	auto shortRangeCondition = [this](const Point3D point, const Facing4) -> Point3D
	{
		if(!m_pointCounter)
			return point;
		m_lastPoint = point;
		return Point3D::null();
	};
	// TODO:(optimization) I don't see a way to write a long range condition here, the whole method should probably be rewritten.
	auto longRangeCondition = [](const Cuboid) -> bool { return true; };
	constexpr bool useAnyOccupiedPoint = false;
	constexpr bool useAdjacent = false;
	return hasPaths.pathToCondition<useAdjacent, useAnyOccupiedPoint>(longRangeCondition, shortRangeCondition, toParamaters(area));
}
void WanderPathRequest::writeStep(Area& area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(path.empty())
	{
		if(m_lastPoint == actors.getLocation(actorIndex) || useCurrentLocation)
			actors.wait(actorIndex, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective);
		else
		{
			m_objective.m_destination = m_lastPoint;
			actors.move_setDestination(actorIndex, m_objective.m_destination);
		}
	}
	else
	{
		m_objective.m_destination = path.back();
		actors.move_setPath(actorIndex, path);
	}
}
Json WanderPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = &m_objective;
	output["lastPoint"] = m_lastPoint;
	output["type"] = "wander";
	return output;
}
// Objective.
WanderObjective::WanderObjective() : Objective(Priority::create(0)) { }
WanderObjective::WanderObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo) { }
Json WanderObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_destination.exists())
		data["destination"] = m_destination;
	return data;
}
void WanderObjective::execute(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_destination.exists())
	{
		if(actors.getLocation(actor) == m_destination)
			actors.objective_complete(actor, *this);
		else
			actors.move_setDestination(actor, m_destination);
	}
	else
		area.getActors().move_pathRequestRecord(actor, std::make_unique<WanderPathRequest>(area, *this, actor));
}
void WanderObjective::cancel(Area& area, const ActorIndex actor) { area.getActors().move_pathRequestMaybeCancel(actor); }
bool WanderObjective::hasPathRequest(const Area& area, const ActorIndex actor) const { return area.getActors().move_hasPathRequest(actor); }
void WanderObjective::reset(Area& area, const ActorIndex actor)
{
	cancel(area, actor);
	area.getActors().canReserve_clearAll(actor);
}
