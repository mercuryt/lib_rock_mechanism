#include "eat.h"
#include "../area.h"
#include "../itemType.h"

EatEvent::EatEvent(Area& area, const Step delay, EatObjective& eo, const Step start) :
       	ScheduledEvent(area.m_simulation, delay, start), m_eatObjective(eo) { }

void EatEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_eatObjective.m_actor;
	BlockIndex blockContainingFood = getBlockWithMostDesiredFoodInReach(*area);
	if(blockContainingFood == BLOCK_INDEX_MAX)
	{
		actors.objective_canNotCompleteSubobjective(actor);
		return;
	}
	Blocks& blocks = area->getBlocks();
	Items& items = area->getItems();
	for(ItemIndex item : blocks.item_getAll(blockContainingFood))
	{
		if(items.isPreparedMeal(item))
		{
			eatPreparedMeal(*area, item);
			return;
		}
		const ItemType& itemType = items.getItemType(item);
		const AnimalSpecies& species = actors.getSpecies(actor);
		if(itemType.edibleForDrinkersOf == &species.fluidType)
		{
			eatGenericItem(*area, item);
			return;
		}
	}
	const AnimalSpecies& species = actors.getSpecies(actor);
	if(species.eatsMeat)
		for(ActorIndex actor : blocks.actor_getAll(blockContainingFood))
			if(!actors.isAlive(actor) && actors.eat_canEatActor(m_eatObjective.m_actor, actor))
			{
				eatActor(*area, actor);
				return;
			}
	if(blocks.plant_exists(blockContainingFood))
	{
		PlantIndex plant = blocks.plant_get(blockContainingFood);
		Plants& plants = area->getPlants();
		const PlantSpecies& plantSpecies = plants.getSpecies(plant);
		if(species.eatsFruit && plantSpecies.fluidType == species.fluidType && plants.getFruitMass(plant) != 0)
			eatFruitFromPlant(*area, plant);
		else if(species.eatsLeaves && plantSpecies.fluidType == species.fluidType && plants.getFoliageMass(plant) != 0)
			eatPlantLeaves(*area, plant);
	}
}
void EatEvent::clearReferences(Simulation&, Area*) { m_eatObjective.m_eatEvent.clearPointer(); }
BlockIndex EatEvent::getBlockWithMostDesiredFoodInReach(Area& area) const
{
	BlockIndex found = BLOCK_INDEX_MAX;
	uint32_t highestDesirability = 0;
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
		uint32_t blockDesirability = mustEat.getDesireToEatSomethingAt(area, block);
		if(blockDesirability == UINT32_MAX)
			return true;
		if(blockDesirability >= mustEat.getMinimumAcceptableDesire() && blockDesirability > highestDesirability)
		{
			found = block;
			highestDesirability = blockDesirability;
		}
		return false;
	};
	BlockIndex output = area.getActors().getBlockWhichIsAdjacentWithPredicate(m_eatObjective.m_actor, predicate);
	if(output == BLOCK_INDEX_MAX)
	       output = found;	
	return output;
}
void EatEvent::eatPreparedMeal(Area& area, ItemIndex item)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	assert(actors.eat_canEatItem(m_eatObjective.m_actor, item));
	assert(items.isPreparedMeal(item));
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), items.getMass(item));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	items.destroy(item);
}
void EatEvent::eatGenericItem(Area& area, ItemIndex item)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	assert(items.getItemType(item).edibleForDrinkersOf == &actors.drink_getFluidType(m_eatObjective.m_actor));
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
	uint32_t quantityDesired = std::ceil((float)mustEat.getMassFoodRequested() / (float)items.getSingleUnitMass(item));
	uint32_t quantityEaten = std::min(quantityDesired, items.getQuantity(item));
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), quantityEaten * items.getSingleUnitMass(item));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	items.removeQuantity(item, quantityEaten);
	if(items.getQuantity(item) == 0)
		items.destroy(item);
}
void EatEvent::eatActor(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	assert(!actors.isAlive(actor));
	assert(actors.getMass(actor) != 0);
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
	Mass massEaten = std::min(actors.getMass(actor), mustEat.getMassFoodRequested());
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	actors.removeMassFromCorpse(actor, massEaten);
}
void EatEvent::eatPlantLeaves(Area& area, PlantIndex plant)
{
	Plants& plants = area.getPlants();
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), plants.getFoliageMass(plant));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	plants.removeFoliageMass(plant, massEaten);
}
void EatEvent::eatFruitFromPlant(Area& area, PlantIndex plant)
{
	Plants& plants = area.getPlants();
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), plants.getFruitMass(plant));
	static const MaterialType& fruitType = MaterialType::byName("fruit");
	uint32_t unitMass = plants.getSpecies(plant).harvestData->fruitItemType.volume * fruitType.density;
	uint32_t quantityEaten = massEaten / unitMass;
	assert(quantityEaten != 0);
	mustEat.eat(area, quantityEaten * unitMass);
	plants.removeFruitQuantity(plant, quantityEaten);
}
EatPathRequest::EatPathRequest(Area& area, EatObjective& eo) : m_eatObjective(eo)
{
	assert(m_eatObjective.m_destination == BLOCK_INDEX_MAX);
	Blocks& blocks = area.getBlocks();
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_eatObjective.m_actor).get();
	std::function<bool(BlockIndex)> predicate = nullptr;
	if(m_eatObjective.m_tryToHunt)
	{
		predicate = [&](BlockIndex block)
		{
			for(ActorIndex actor : blocks.actor_getAll(block))
				if(mustEat.canEatActor(area, actor))
				{
					m_huntResult = actor;
					return true;
				}
			return false;
		};
	}
	else
	{
		// initalize candidates with null values.
		m_candidates.fill(BLOCK_INDEX_MAX);
		predicate = [&mustEat, this, &area](BlockIndex block)
		{
			uint32_t eatDesire = mustEat.getDesireToEatSomethingAt(area, block);
			if(eatDesire == UINT32_MAX)
				return true;
			if(eatDesire < mustEat.getMinimumAcceptableDesire())
				return false;
			if(eatDesire != 0 && m_candidates[eatDesire - 1u] == BLOCK_INDEX_MAX)
				m_candidates[eatDesire - 1u] = block;
			return false;
		};
	}
	//TODO: maxRange.
	bool unreserved = false;
	bool reserve = false;
	if(area.getActors().getFaction(m_eatObjective.m_actor) != nullptr)
		unreserved = reserve = true;
	createGoAdjacentToCondition(area, m_eatObjective.m_actor, predicate, m_eatObjective.m_detour, unreserved, BLOCK_DISTANCE_MAX, BLOCK_INDEX_MAX);
}
void EatPathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_eatObjective.m_actor;
	const AnimalSpecies species = actors.getSpecies(actor);
	if(m_eatObjective.m_tryToHunt)
	{
		if(m_huntResult == ACTOR_INDEX_MAX)
		{
			m_eatObjective.m_noFoodFound = true;
			m_eatObjective.execute(area);
			return;
		}
		std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(m_eatObjective.m_actor, m_huntResult);
		actors.m_hasObjectives.at(actor).addNeed(area, std::move(killObjective));
	}
	else
	{
		if(result.path.empty() && !result.useCurrentPosition)
		{
			if(species.eatsMeat)
				m_eatObjective.m_tryToHunt = true;
			else
				m_eatObjective.m_noFoodFound = true;
			m_eatObjective.execute(area);
			return;
		}
		Faction* faction = actors.getFaction(actor);
		if(faction != nullptr)
		{
			if(result.useCurrentPosition)
			{
				if(!actors.move_tryToReserveOccupied(actor))
				{
					m_eatObjective.execute(area);
					return;
				}
			}
			else if(!actors.move_tryToReserveDestination(actor))
			{
				m_eatObjective.reset(area);
				m_eatObjective.execute(area);
				return;
			}
		}
		m_eatObjective.m_destination = result.path.back();
		actors.move_setPath(m_eatObjective.m_actor, result.path);
	}
}
EatObjective::EatObjective(Area& area, ActorIndex a) :
	Objective(a, Config::eatPriority), m_eatEvent(area.m_eventSchedule) { }
/*
EatObjective::EatObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_eatEvent(deserializationMemo.m_simulation.m_eventSchedule), 
	m_noFoodFound(data["noFoodFound"].get<bool>())
{
	if(data.contains("destination"))
		m_destination = data["destination"].get<BlockIndex>();
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_eatEvent.schedule(Config::stepsToEat, *this, data["eventStart"].get<Step>());
}
Json EatObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noFoodFound"] = m_noFoodFound;
	if(m_destination != BLOCK_INDEX_MAX)
		data["destination"] = m_destination;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_eatEvent.exists())
		data["eatStart"] = m_eatEvent.getStartStep();
	return data;
}
*/
void EatObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	MustEat& mustEat = *area.getActors().m_mustEat.at(m_actor).get();
	if(m_noFoodFound)
	{
		// We have determined that there is no food here and have attempted to path to the edge of the area so we can leave.
		if(actors.predicateForAnyOccupiedBlock(m_actor, [&area](BlockIndex block){ return area.getBlocks().isEdge(block); }))
			// We are at the edge and can leave.
			actors.leaveArea(m_actor);
		else
			// No food and no escape.
			actors.objective_canNotFulfillNeed(m_actor, *this);
		return;
	}
	BlockIndex adjacent = mustEat.getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability(area);
	if(m_destination == BLOCK_INDEX_MAX)
	{
		if(adjacent == BLOCK_INDEX_MAX)
			// Find destination.
			makePathRequest(area);
		else
		{
			m_destination = actors.getLocation(m_actor);
			// Start eating.
			// TODO: reserve occupied?
			m_eatEvent.schedule(Config::stepsToEat, *this);
		}
	}
	else
	{	
		if(actors.getLocation(m_actor) == m_destination)
		{
			if(adjacent == BLOCK_INDEX_MAX)
			{
				// We are at the previously selected location but there is no  longer any food here, try again.
				m_destination = BLOCK_INDEX_MAX;
				actors.canReserve_clearAll(m_actor);
				makePathRequest(area);
			}
			else
				// Start eating.
				m_eatEvent.schedule(Config::stepsToEat, *this);
		}
		else
			actors.move_setDestination(m_actor, m_destination, m_detour);
	}
}
void EatObjective::cancel(Area& area)
{
	Actors& actors = area.m_actors;
	actors.move_pathRequestMaybeCancel(m_actor);
	m_eatEvent.maybeUnschedule();
	actors.m_mustEat.at(m_actor)->m_eatObjective = nullptr;
}
void EatObjective::delay(Area& area)
{
	area.getActors().move_pathRequestMaybeCancel(m_actor);
	m_eatEvent.maybeUnschedule();
}
void EatObjective::reset(Area& area)
{
	delay(area);
	m_destination = BLOCK_INDEX_MAX;
	m_noFoodFound = false;
	area.getActors().canReserve_clearAll(m_actor);
}
void EatObjective::makePathRequest(Area& area)
{
	std::unique_ptr<PathRequest> request = std::make_unique<EatPathRequest>(area, *this);
	area.getActors().move_pathRequestRecord(m_actor, std::move(request));
}
void EatObjective::noFoodFound()
{
	m_noFoodFound = true;
}
bool EatObjective::canEatAt(Area& area, BlockIndex block) const
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
	{
		if(actors.eat_canEatItem(m_actor, item))
			return true;
		if(items.getItemType(item).internalVolume != 0)
			for(ItemIndex i : items.cargo_getItems(item))
				if(actors.eat_canEatItem(m_actor, i))
					return true;
	}
	const AnimalSpecies& species = actors.getSpecies(m_actor);
	if(species.eatsMeat)
		for(const ActorIndex actor : blocks.actor_getAll(block))
			if(!actors.isAlive(actor) && species.fluidType == actors.getSpecies(actor).fluidType )
				return true;
	if(blocks.plant_exists(block))
	{
		const PlantIndex plant = blocks.plant_get(block);
		if(species.eatsFruit && area.getPlants().getSpecies(plant).fluidType == species.fluidType)
			if(actors.eat_canEatPlant(m_actor, blocks.plant_get(block)))
				return true;
	}
	return false;
}
