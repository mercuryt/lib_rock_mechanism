#include "harvest.h"
#include "../area/area.h"
#include "../config.h"
#include "../farmFields.h"
#include "../objective.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../space/space.h"
#include "../plants.h"
#include "../hasShapes.hpp"
#include "../path/terrainFacade.hpp"
#include "../path/pathRequest.h"
// Event.
HarvestEvent::HarvestEvent(const Step& delay, Area& area, HarvestObjective& ho, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_harvestObjective(ho)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void HarvestEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	assert(m_harvestObjective.m_point.exists());
	assert(actors.isAdjacentToLocation(actor, m_harvestObjective.m_point));
	Space& space = area->getSpace();
	if(!space.plant_exists(m_harvestObjective.m_point))
		// Plant no longer exsits, try again.
		m_harvestObjective.makePathRequest(*area, actor);
	Plants& plants = area->getPlants();
	PlantIndex plant = space.plant_get(m_harvestObjective.m_point);
	static MaterialTypeId plantMatter = MaterialType::byName("plant matter");
	ItemTypeId fruitItemType = PlantSpecies::getFruitItemType(plants.getSpecies(plant));
	Quantity quantityToHarvest = plants.getQuantityToHarvest(plant);
	if(quantityToHarvest == 0)
		actors.objective_canNotCompleteSubobjective(actor);
	else
	{
		Quantity numberItemsHarvested = std::min(quantityToHarvest, actors.canPickUp_quantityWhichCanBePickedUpUnencombered(actor, fruitItemType, plantMatter));
		assert(numberItemsHarvested != 0);
		//TODO: apply horticulture skill.
		plants.harvest(plant, numberItemsHarvested);
		space.item_addGeneric(actors.getLocation(actor), fruitItemType, plantMatter, numberItemsHarvested);
		actors.objective_complete(actor, m_harvestObjective);
	}
}
void HarvestEvent::clearReferences(Simulation&, Area*) { m_harvestObjective.m_harvestEvent.clearPointer(); }
// Objective type.
bool HarvestObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	// Pilots and passengers onDeck cannot harvest.
	if(actors.onDeck_getIsOnDeckOf(actor).exists())
		return false;
	return area.m_hasFarmFields.hasHarvestDesignations(area.getActors().getFaction(actor));
}
std::unique_ptr<Objective> HarvestObjectiveType::makeFor(Area& area, const ActorIndex&) const
{
	return std::make_unique<HarvestObjective>(area);
}
// Objective.
HarvestObjective::HarvestObjective(Area& area) :
	Objective(Config::harvestPriority), m_harvestEvent(area.m_eventSchedule) { }
HarvestObjective::HarvestObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_harvestEvent(area.m_eventSchedule)
{
	if(data.contains("eventStart"))
		m_harvestEvent.schedule(Config::harvestEventDuration, area, *this, data["actor"].get<ActorIndex>(), data["eventStart"].get<Step>());
	if(data.contains("point"))
		m_point = data["point"].get<Point3D>();
}
Json HarvestObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_point.exists())
		data["point"] = m_point;
	if(m_harvestEvent.exists())
		data["eventStart"] = m_harvestEvent.getStartStep();
	return data;
}
void HarvestObjective::execute(Area& area, const ActorIndex& actor)
{
	Space& space =  area.getSpace();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	if(m_point.exists())
	{
		//TODO: Area level listing of plant species to not harvest.
		if(actors.isAdjacentToLocation(actor, m_point) && space.plant_exists(m_point) && plants.readyToHarvest(space.plant_get(m_point)))
			begin(area, actor);
		else
		{
			// Previously found point or route is no longer valid, redo from start.
			reset(area, actor);
			execute(area, actor);
		}
	}
	else
	{
		Point3D point = getPointContainingPlantToHarvestAtLocationAndFacing(area, actors.getLocation(actor), actors.getFacing(actor), actor);
		if(point.exists() && space.plant_exists(m_point) && plants.readyToHarvest(space.plant_get(m_point)) && actors.move_tryToReserveOccupied(actor))
		{
			select(area, point, actor);
			begin(area, actor);
			return;
		}
		makePathRequest(area, actor);
	}
}
void HarvestObjective::cancel(Area& area, const ActorIndex& actor)
{

	Actors& actors = area.getActors();
	Space& space =  area.getSpace();
	Plants& plants = area.getPlants();
	actors.move_pathRequestMaybeCancel(actor);
	m_harvestEvent.maybeUnschedule();
	if(m_point.exists() && space.plant_exists(m_point) && plants.readyToHarvest(space.plant_get(m_point)))
		area.m_hasFarmFields.getForFaction(actors.getFaction(actor)).addHarvestDesignation(area, space.plant_get(m_point));
}
void HarvestObjective::select(Area& area, const Point3D& point, const ActorIndex& actor)
{
	Space& space =  area.getSpace();
	[[maybe_unused]] Plants& plants = area.getPlants();
	Actors& actors = area.getActors();
	assert(space.plant_exists(point));
	assert(plants.readyToHarvest(space.plant_get(point)));
	m_point = point;
	area.m_hasFarmFields.getForFaction(actors.getFaction(actor)).removeHarvestDesignation(area, space.plant_get(point));
}
void HarvestObjective::begin(Area& area, const ActorIndex& actor)
{
	[[maybe_unused]] Plants& plants = area.getPlants();
	assert(m_point.exists());
	assert(plants.readyToHarvest( area.getSpace().plant_get(m_point)));
	m_harvestEvent.schedule(Config::harvestEventDuration, area, *this, actor);
}
void HarvestObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_point.clear();
}
void HarvestObjective::makePathRequest(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<HarvestPathRequest>(area, *this, actor));
}
Point3D HarvestObjective::getPointContainingPlantToHarvestAtLocationAndFacing(Area& area, const Point3D& location, Facing4 facing, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	std::function<bool(const Point3D&)> predicate = [&](const Point3D& point) { return pointContainsHarvestablePlant(area, point, actor); };
	return actors.getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
bool HarvestObjective::pointContainsHarvestablePlant(Area& area, const Point3D& point, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	Space& space =  area.getSpace();
	Plants& plants = area.getPlants();
	return space.plant_exists(point) && plants.readyToHarvest(space.plant_get(point)) && !space.isReserved(point, actors.getFaction(actor));
}
HarvestPathRequest::HarvestPathRequest(Area& area, HarvestObjective& objective, const ActorIndex& actorIndex) :
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
HarvestPathRequest::HarvestPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<HarvestObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{ }
FindPathResult HarvestPathRequest::readStep(Area&, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	constexpr bool unreserved = false;
	return terrainFacade.findPathToSpaceDesignation(memo, SpaceDesignation::Harvest, faction, start, facing, shape, m_objective.m_detour, adjacent, unreserved, Config::maxRangeToSearchForHorticultureDesignations);
}
void HarvestPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	const Point3D& harvestLocation = result.pointThatPassedPredicate;
	if(result.path.empty() && result.pointThatPassedPredicate.empty())
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
	else if(!area.m_spaceDesignations.getForFaction(faction).check(harvestLocation, SpaceDesignation::Harvest))
		// Retry.
		actors.objective_canNotCompleteSubobjective(actorIndex);
	else
	{
		m_objective.select(area, result.pointThatPassedPredicate, actorIndex);
		actors.move_setPath(actorIndex, result.path);
	}
}
Json HarvestPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "harvest";
	return output;
}