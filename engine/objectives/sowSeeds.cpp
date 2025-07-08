#include "sowSeeds.h"
#include "../area/area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../simulation/simulation.h"
#include "../hasShapes.hpp"
#include "../path/terrainFacade.hpp"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../designations.h"
#include "../numericTypes/types.h"

struct DeserializationMemo;

SowSeedsEvent::SowSeedsEvent(const Step& delay, Area& area, SowSeedsObjective& o, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_objective(o)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void SowSeedsEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	Space& space = area->getSpace();
	Point3D point = m_objective.m_point;
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	FactionId faction = actors.getFaction(actor);
	if(!space.farm_contains(point, faction))
	{
		// Point3D is no longer part of a field. It may have been undesignated or it may no longer be a suitable place to grow the selected plant.
		m_objective.reset(*area, actor);
		m_objective.execute(*area, actor);
	}
	assert(space.farm_get(point, faction));
	PlantSpeciesId plantSpecies = space.farm_get(point, faction)->plantSpecies;
	space.plant_create(point, plantSpecies, Percent::create(0));
	actors.objective_complete(actor, m_objective);
}
void SowSeedsEvent::clearReferences(Simulation&, Area*){ m_objective.m_event.clearPointer(); }
bool SowSeedsObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot sow.
	if(actors.onDeck_getIsOnDeckOf(actor).exists())
		return false;
	return area.m_hasFarmFields.hasSowSeedsDesignations(area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> SowSeedsObjectiveType::makeFor(Area& area, const ActorIndex&) const
{
	return std::make_unique<SowSeedsObjective>(area);
}
SowSeedsObjective::SowSeedsObjective(Area& area) :
	Objective(Config::sowSeedsPriority),
	m_event(area.m_eventSchedule) { }
SowSeedsObjective::SowSeedsObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_event(area.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_event.schedule(Config::sowSeedsStepsDuration, area, *this, actor, data["eventStart"].get<Step>());
	if(data.contains("point"))
		m_point = data["point"].get<Point3D>();
}
Json SowSeedsObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_point.exists())
		data["point"] = m_point;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
Point3D SowSeedsObjective::getPointToSowAt(Area& area, const Point3D& location, Facing4 facing, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFaction(actor);
	AreaHasSpaceDesignationsForFaction& designations = area.m_spaceDesignations.getForFaction(faction);
	Space& space = area.getSpace();
	std::function<bool(const Point3D&)> predicate = [&](const Point3D& point)
	{
		return designations.check(point, SpaceDesignation::SowSeeds) && !space.isReserved(point, faction);
	};
	return actors.getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
void SowSeedsObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	Space& space =  area.getSpace();
	if(m_point.exists())
	{
		if(actors.isAdjacentToLocation(actor, m_point))
		{
			FarmField* field = space.farm_get(m_point, actors.getFaction(actor));
			if(field != nullptr && space.plant_canGrowHereAtSomePointToday(m_point, field->plantSpecies))
			{
				begin(area, actor);
				return;
			}
		}
		// Previously found m_point or path no longer valid, redo from start.
		reset(area, actor);
		execute(area, actor);
		return;
	}
	else
	{
		// Check if we can use an adjacent as m_point.
		if(actors.allOccupiedPointsAreReservable(actor, actors.getFaction(actor)))
		{
			Point3D point = getPointToSowAt(area, actors.getLocation(actor), actors.getFacing(actor), actor);
			if(point.exists())
			{
				select(area, point, actor);
				assert(actors.move_tryToReserveOccupied(actor));
				begin(area, actor);
				return;
			}
		}
	}
	actors.move_pathRequestRecord(actor, std::make_unique<SowSeedsPathRequest>(area, *this, actor));
}
void SowSeedsObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_event.maybeUnschedule();
	auto& space =  area.getSpace();
	FactionId faction = actors.getFaction(actor);
	if(m_point.exists() && space.farm_contains(m_point, faction))
	{
		FarmField* field = space.farm_get(m_point, faction);
		if(field == nullptr)
			return;
		//TODO: check it is still planting season.
		if(space.plant_canGrowHereAtSomePointToday(m_point, field->plantSpecies))
			area.m_hasFarmFields.getForFaction(faction).addSowSeedsDesignation(area, m_point);
	}
}
void SowSeedsObjective::select(Area& area, const Point3D& point, const ActorIndex& actor)
{
	[[maybe_unused]] Space& space =  area.getSpace();
	[[maybe_unused]] Actors& actors = area.getActors();
	assert(!space.plant_exists(point));
	assert(space.farm_contains(point, actors.getFaction(actor)));
	assert(m_point.empty());
	m_point = point;
	area.m_hasFarmFields.getForFaction(actors.getFaction(actor)).removeSowSeedsDesignation(area, point);
}
void SowSeedsObjective::begin(Area& area, const ActorIndex& actor)
{
	[[maybe_unused]] Actors& actors = area.getActors();
	assert(m_point.exists());
	assert(actors.isAdjacentToLocation(actor, m_point));
	m_event.schedule(Config::sowSeedsStepsDuration, area, *this, actor);
}
void SowSeedsObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_point.clear();
}
bool SowSeedsObjective::canSowAt(Area& area, const Point3D& point, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	FactionId faction = actors.getFaction(actor);
	auto& space =  area.getSpace();
	return space.designation_has(point, faction, SpaceDesignation::SowSeeds) && !space.isReserved(point, faction);
}
SowSeedsPathRequest::SowSeedsPathRequest(Area& area, SowSeedsObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
SowSeedsPathRequest::SowSeedsPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<SowSeedsObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
FindPathResult SowSeedsPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	constexpr bool unreserved = false;
	return terrainFacade.findPathToSpaceDesignation(memo, SpaceDesignation::SowSeeds, actors.getFaction(actorIndex), actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_objective.m_detour, adjacent, unreserved, Config::maxRangeToSearchForHorticultureDesignations);
}
void SowSeedsPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	const Point3D& sowLocation = result.pointThatPassedPredicate;
	if(result.path.empty() && sowLocation.empty())
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
	else if(!area.m_spaceDesignations.getForFaction(faction).check(sowLocation, SpaceDesignation::SowSeeds))
		// Retry. The location is no longer designated.
		actors.objective_canNotCompleteSubobjective(actorIndex);
	else
	{
		m_objective.select(area, result.pointThatPassedPredicate, actorIndex);
		actors.move_setPath(actorIndex, result.path);
	}
}
Json SowSeedsPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = name();
	return output;
}