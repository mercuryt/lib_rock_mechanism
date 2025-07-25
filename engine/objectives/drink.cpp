#include "drink.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../hasShapes.hpp"
#include "../path/terrainFacade.hpp"

// Drink Threaded Task.
DrinkPathRequest::DrinkPathRequest(Area& area, DrinkObjective& drob, const ActorIndex& actorIndex) :
	m_drinkObjective(drob)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Distance::max();
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_drinkObjective.m_detour;
	adjacent = true;
}
FindPathResult DrinkPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	if(m_drinkObjective.m_noDrinkFound)
		return terrainFacade.findPathToEdge(memo, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_drinkObjective.m_detour);
	auto destinationCondition = [this, &area, actorIndex](const Point3D& point, const Facing4&) -> std::pair<bool, Point3D>
	{
		return {m_drinkObjective.containsSomethingDrinkable(area, point, actorIndex), point};
	};
	constexpr bool useAnyPoint = true;
	return terrainFacade.findPathToConditionBreadthFirst<useAnyPoint, decltype(destinationCondition)>(destinationCondition, memo, actors.getLocation(actorIndex), actors.getFacing(actorIndex), actors.getShape(actorIndex), m_drinkObjective.m_detour, adjacent, actors.getFaction(actorIndex), Distance::max());
}
DrinkPathRequest::DrinkPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_drinkObjective(static_cast<DrinkObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))) { }
void DrinkPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty())
	{
		if(!result.useCurrentPosition)
		{
			if(m_drinkObjective.m_noDrinkFound)
				actors.objective_canNotFulfillNeed(actorIndex, m_drinkObjective);
			else
			{
				// Nothing to drink here, try to leave.
				m_drinkObjective.m_noDrinkFound = true;
				m_drinkObjective.execute(area, actorIndex);
			}
		}
		else
		{
			// There is something to drink at the current location.
			m_drinkObjective.m_noDrinkFound = false;
			m_drinkObjective.execute(area, actorIndex);
		}
	}
	else
		actors.move_setPath(actorIndex, result.path);
}
Json DrinkPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_drinkObjective;
	output["type"] = "drink";
	return output;
}
// Drink Objective.
DrinkObjective::DrinkObjective(Area& area) :
	Objective(Config::drinkPriority), m_drinkEvent(area.m_eventSchedule) { }
DrinkObjective::DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const ActorIndex& actor) :
	Objective(data, deserializationMemo), m_drinkEvent(area.m_eventSchedule), m_noDrinkFound(data["noDrinkFound"].get<bool>())
{
	if(data.contains("eventStart"))
		m_drinkEvent.schedule(area, Config::stepsToDrink, *this, actor, data["eventStart"].get<Step>());
}
Json DrinkObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noDrinkFound"] = m_noDrinkFound;
	if(m_drinkEvent.exists())
		data["eventStart"] = m_drinkEvent.getStartStep();
	return data;
}
void DrinkObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	MustDrink &mustDrink = *area.getActors().m_mustDrink[actor].get();
	// If the objective was supressed and then unsupressed MustDrink::m_objective needs to be restored.
	if (!mustDrink.hasObjective())
		mustDrink.setObjective(*this);
	if(m_noDrinkFound)
	{
		// We have determined that there is no drink here and have attempted to path to the edge of the area so we can leave.
		if(actors.isOnEdge(actor))
			// We are at the edge and can leave.
			actors.leaveArea(actor);
		else
			// Try to get to the edge.
			makePathRequest(area, actor);
		return;
	}
	if(!canDrinkAt(area, actors.getLocation(actor), actors.getFacing(actor), actor))
		makePathRequest(area, actor);
	else
		m_drinkEvent.schedule(area, Config::stepsToDrink, *this, actor);
}
void DrinkObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	m_drinkEvent.maybeUnschedule();
	actors.m_mustDrink[actor]->m_objective = nullptr;
}
void DrinkObjective::delay(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
	m_drinkEvent.maybeUnschedule();
}
void DrinkObjective::reset(Area& area, const ActorIndex& actor)
{
	delay(area, actor);
	m_noDrinkFound = false;
}
void DrinkObjective::makePathRequest(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<DrinkPathRequest>(area, *this, actor));
}
bool DrinkObjective::canDrinkAt(Area& area, const Point3D& point, const Facing4& facing, const ActorIndex& actor) const
{
	return getAdjacentPointToDrinkAt(area, point, facing, actor).exists();
}
Point3D DrinkObjective::getAdjacentPointToDrinkAt(Area& area, const Point3D& location, const Facing4& facing, const ActorIndex& actor) const
{
	std::function<bool(const Point3D&)> predicate = [&](const Point3D& point) { return containsSomethingDrinkable(area, point, actor); };
	return area.getActors().getPointWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
bool DrinkObjective::canDrinkItemAt(Area& area, const Point3D& point, const ActorIndex& actor) const
{
	return getItemToDrinkFromAt(area, point, actor) != ItemIndex::null();
}
ItemIndex DrinkObjective::getItemToDrinkFromAt(Area& area, const Point3D& point, const ActorIndex& actor) const
{
	Items& items = area.getItems();
	for(ItemIndex item :  area.getSpace().item_getAll(point))
		if(items.cargo_containsAnyFluid(item) && items.cargo_getFluidType(item) == area.getActors().drink_getFluidType(actor))
			return item;
	return ItemIndex::null();
}
bool DrinkObjective::containsSomethingDrinkable(Area& area, const Point3D& point, const ActorIndex& actor) const
{
	return  area.getSpace().fluid_contains(point, area.getActors().drink_getFluidType(actor)) || canDrinkItemAt(area, point, actor);
}
