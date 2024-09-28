#include "drink.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"

// Drink Threaded Task.
DrinkPathRequest::DrinkPathRequest(Area& area, DrinkObjective& drob, ActorIndex actor) : m_drinkObjective(drob)
{
	std::function<bool(BlockIndex)> predicate = [this, &area, actor](BlockIndex block)
	{
		return m_drinkObjective.containsSomethingDrinkable(area, block, actor);
	};
	bool reserve = false;
	bool unreserved = false;
	createGoAdjacentToCondition(area, actor, predicate, drob.m_detour, unreserved, DistanceInBlocks::null(), BlockIndex::null(), reserve);
}
DrinkPathRequest::DrinkPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_drinkObjective(static_cast<DrinkObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))),
	m_noDrinkFound(data["noDrinkFound"].get<bool>())
{
	nlohmann::from_json(data, *this);
}
void DrinkPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(result.path.empty())
	{
		if(!result.useCurrentPosition)
		{
			if(m_noDrinkFound)
				actors.objective_canNotFulfillNeed(getActor(), m_drinkObjective);
			else
			{
				// Nothing to drink here, try to leave.
				m_noDrinkFound = true;
				createGoToEdge(area, getActor(), m_drinkObjective.m_detour);
			}
		}
		else
			actors.objective_subobjectiveComplete(getActor());
	}
	else
		actors.move_setPath(getActor(), result.path);
	if(m_noDrinkFound)
		m_drinkObjective.m_noDrinkFound = true;
}
Json DrinkPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_drinkObjective;
	output["noDrinkFound"] = m_noDrinkFound;
	return output;
}
// Drink Objective.
DrinkObjective::DrinkObjective(Area& area) :
	Objective(Config::drinkPriority), m_drinkEvent(area.m_eventSchedule) { }
DrinkObjective::DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, ActorIndex actor) :
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
void DrinkObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_noDrinkFound)
	{
		// We have determined that there is no drink here and have attempted to path to the edge of the area so we can leave.
		if(actors.predicateForAnyOccupiedBlock(actor, [&area](BlockIndex block){ return area.getBlocks().isEdge(block); }))
			// We are at the edge and can leave.
			actors.leaveArea(actor);
		else
			// No drink and no escape.
			actors.objective_canNotFulfillNeed(actor, *this);
		return;
	}
	if(!canDrinkAt(area, actors.getLocation(actor), actors.getFacing(actor), actor))
		makePathRequest(area, actor);
	else
		m_drinkEvent.schedule(area, Config::stepsToDrink, *this, actor);
}
void DrinkObjective::cancel(Area& area, ActorIndex actor) 
{ 
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	m_drinkEvent.maybeUnschedule();
	actors.m_mustDrink[actor]->m_objective = nullptr;
}
void DrinkObjective::delay(Area& area, ActorIndex actor) 
{ 
	area.getActors().move_pathRequestMaybeCancel(actor);
	m_drinkEvent.maybeUnschedule();
}
void DrinkObjective::reset(Area& area, ActorIndex actor)
{
	delay(area, actor);
	m_noDrinkFound = false;
}
void DrinkObjective::makePathRequest(Area& area, ActorIndex actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<DrinkPathRequest>(area, *this, actor));
}
bool DrinkObjective::canDrinkAt(Area& area, BlockIndex block, Facing facing, ActorIndex actor) const
{
	return getAdjacentBlockToDrinkAt(area, block, facing, actor).exists();
}
BlockIndex DrinkObjective::getAdjacentBlockToDrinkAt(Area& area, BlockIndex location, Facing facing, ActorIndex actor) const
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return containsSomethingDrinkable(area, block, actor); };
	return area.getActors().getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(actor, location, facing, predicate);
}
bool DrinkObjective::canDrinkItemAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	return getItemToDrinkFromAt(area, block, actor) != ItemIndex::null();
}
ItemIndex DrinkObjective::getItemToDrinkFromAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	Items& items = area.getItems();
	for(ItemIndex item : area.getBlocks().item_getAll(block))
		if(items.cargo_containsAnyFluid(item) && items.cargo_getFluidType(item) == area.getActors().drink_getFluidType(actor))
			return item;
	return ItemIndex::null();
}
bool DrinkObjective::containsSomethingDrinkable(Area& area, BlockIndex block, ActorIndex actor) const
{
	return area.getBlocks().fluid_contains(block, area.getActors().drink_getFluidType(actor)) || canDrinkItemAt(area, block, actor);
}
