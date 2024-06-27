#include "drink.h"
#include "../area.h"
// Drink Threaded Task.
DrinkPathRequest::DrinkPathRequest(Area& area, DrinkObjective& drob) : m_drinkObjective(drob)
{
	std::function<bool(BlockIndex)> predicate = [this, &area](BlockIndex block)
	{
		return m_drinkObjective.containsSomethingDrinkable(area, block);
	};
	bool reserve = false;
	bool unreserved = false;
	if(area.getActors().getFaction(m_drinkObjective.m_actor))
	{
		reserve = true;
		unreserved = true;
	}
	createGoAdjacentToCondition(area, m_drinkObjective.m_actor, predicate, drob.m_detour, unreserved, BLOCK_DISTANCE_MAX, BLOCK_INDEX_MAX, reserve);
}
void DrinkPathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	if(result.path.empty())
	{
		if(!result.useCurrentPosition)
		{
			if(m_noDrinkFound)
				actors.objective_canNotFulfillNeed(m_drinkObjective.m_actor, m_drinkObjective);
			else
			{
				// Nothing to drink here, try to leave.
				createGoToEdge(area, m_drinkObjective.m_actor, m_drinkObjective.m_detour);
				m_noDrinkFound = true;
			}
		}
		else
			if(!actors.move_tryToReserveOccupied(m_drinkObjective.m_actor))
				m_drinkObjective.makePathRequest(area);
			else
				actors.objective_subobjectiveComplete(m_drinkObjective.m_actor);
	}
	else
	{
		if(!actors.move_tryToReserveDestination(m_drinkObjective.m_actor))
			m_drinkObjective.makePathRequest(area);
		else
			actors.move_setPath(m_drinkObjective.m_actor, result.path);
	}
	// Need task becomes exit area.
	if(m_noDrinkFound)
		m_drinkObjective.m_noDrinkFound = true;
}
// Drink Objective.
DrinkObjective::DrinkObjective(Area& area, ActorIndex a) :
	Objective(a, Config::drinkPriority), m_drinkEvent(area.m_eventSchedule) { }
/*
DrinkObjective::DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_drinkEvent(deserializationMemo.m_simulation.m_eventSchedule), m_noDrinkFound(data["noDrinkFound"].get<bool>())
{ 
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_drinkEvent.schedule(Config::stepsToDrink, *this, data["eventStart"].get<Step>());
}
Json DrinkObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noDrinkFound"] = m_noDrinkFound;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_drinkEvent.exists())
		data["eventStart"] = m_drinkEvent.getStartStep();
	return data;
}
*/
void DrinkObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(m_noDrinkFound)
	{
		// We have determined that there is no drink here and have attempted to path to the edge of the area so we can leave.
		if(actors.predicateForAnyOccupiedBlock(m_actor, [&area](BlockIndex block){ return area.getBlocks().isEdge(block); }))
			// We are at the edge and can leave.
			actors.leaveArea(m_actor);
		else
			// No drink and no escape.
			actors.objective_canNotFulfillNeed(m_actor, *this);
		return;
	}
	if(!canDrinkAt(area, actors.getLocation(m_actor), actors.getFacing(m_actor)))
		makePathRequest(area);
	else
		m_drinkEvent.schedule(Config::stepsToDrink, *this);
}
void DrinkObjective::cancel(Area& area) 
{ 
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(m_actor);
	m_drinkEvent.maybeUnschedule();
	actors.m_mustDrink.at(m_actor)->m_objective = nullptr;
}
void DrinkObjective::delay(Area& area) 
{ 
	area.getActors().move_pathRequestMaybeCancel(m_actor);
	m_drinkEvent.maybeUnschedule();
}
void DrinkObjective::reset(Area& area)
{
	delay(area);
	m_noDrinkFound = false;
	area.getActors().canReserve_clearAll(m_actor);
}
void DrinkObjective::makePathRequest(Area& area)
{
	std::unique_ptr<PathRequest> pathRequest = std::make_unique<DrinkPathRequest>(area, *this);
}
bool DrinkObjective::canDrinkAt(Area& area, BlockIndex block, Facing facing) const
{
	return getAdjacentBlockToDrinkAt(area, block, facing) != BLOCK_INDEX_MAX;
}
BlockIndex DrinkObjective::getAdjacentBlockToDrinkAt(Area& area, BlockIndex location, Facing facing) const
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return containsSomethingDrinkable(area, block); };
	return area.getActors().getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(m_actor, location, facing, predicate);
}
bool DrinkObjective::canDrinkItemAt(Area& area, BlockIndex block) const
{
	return getItemToDrinkFromAt(area, block) != ITEM_INDEX_MAX;
}
ItemIndex DrinkObjective::getItemToDrinkFromAt(Area& area, BlockIndex block) const
{
	Items& items = area.getItems();
	for(ItemIndex item : area.getBlocks().item_getAll(block))
		if(items.cargo_containsAnyFluid(item) && items.cargo_getFluidType(item) == area.getActors().drink_getFluidType(m_actor))
			return item;
	return ITEM_INDEX_MAX;
}
bool DrinkObjective::containsSomethingDrinkable(Area& area, BlockIndex block) const
{
	return area.getBlocks().fluid_contains(block, area.getActors().drink_getFluidType(m_actor)) || canDrinkItemAt(area, block);
}
