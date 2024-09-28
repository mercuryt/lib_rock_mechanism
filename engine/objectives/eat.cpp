#include "eat.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "../items/items.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../itemType.h"
#include "../animalSpecies.h"
#include "kill.h"
#include "types.h"

EatEvent::EatEvent(Area& area, const Step delay, EatObjective& eo, ActorIndex actor, const Step start) :
       	ScheduledEvent(area.m_simulation, delay, start), m_eatObjective(eo)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
void EatEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex();
	BlockIndex blockContainingFood = getBlockWithMostDesiredFoodInReach(*area);
	if(blockContainingFood.empty())
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
		ItemTypeId itemType = items.getItemType(item);
		AnimalSpeciesId species = actors.getSpecies(actor);
		if(ItemType::getEdibleForDrinkersOf(itemType) == AnimalSpecies::getFluidType(species))
		{
			eatGenericItem(*area, item);
			return;
		}
	}
	AnimalSpeciesId species = actors.getSpecies(actor);
	if(AnimalSpecies::getEatsMeat(species))
		for(ActorIndex actorToEat : blocks.actor_getAll(blockContainingFood))
			if(!actors.isAlive(actorToEat) && actors.eat_canEatActor(actor, actorToEat))
			{
				eatActor(*area, actorToEat);
				return;
			}
	if(blocks.plant_exists(blockContainingFood))
	{
		PlantIndex plant = blocks.plant_get(blockContainingFood);
		Plants& plants = area->getPlants();
		PlantSpeciesId plantSpecies = plants.getSpecies(plant);
		if(AnimalSpecies::getEatsFruit(species) && PlantSpecies::getFluidType(plantSpecies) == AnimalSpecies::getFluidType(species) && plants.getFruitMass(plant) != 0)
			eatFruitFromPlant(*area, plant);
		else if(AnimalSpecies::getEatsLeaves(species) && PlantSpecies::getFluidType(plantSpecies) == AnimalSpecies::getFluidType(species) && plants.getFoliageMass(plant) != 0)
			eatPlantLeaves(*area, plant);
	}
}
void EatEvent::clearReferences(Simulation&, Area*) { m_eatObjective.m_eatEvent.clearPointer(); }
BlockIndex EatEvent::getBlockWithMostDesiredFoodInReach(Area& area) const
{
	BlockIndex found;
	uint32_t highestDesirability = 0;
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		MustEat& mustEat = *area.getActors().m_mustEat[m_actor.getIndex()].get();
		uint32_t blockDesirability = mustEat.getDesireToEatSomethingAt(area, block);
		if(blockDesirability == UINT32_MAX)
			return true;
		if(blockDesirability >= mustEat.getMinimumAcceptableDesire(area) && blockDesirability > highestDesirability)
		{
			found = block;
			highestDesirability = blockDesirability;
		}
		return false;
	};
	BlockIndex output = area.getActors().getBlockWhichIsAdjacentWithPredicate(m_actor.getIndex(), predicate);
	if(output.empty())
	       output = found;	
	return output;
}
void EatEvent::eatPreparedMeal(Area& area, ItemIndex item)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	assert(actors.eat_canEatItem(m_actor.getIndex(), item));
	assert(items.isPreparedMeal(item));
	MustEat& mustEat = *area.getActors().m_mustEat[m_actor.getIndex()].get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), items.getMass(item));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	items.destroy(item);
}
void EatEvent::eatGenericItem(Area& area, ItemIndex item)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	assert(ItemType::getEdibleForDrinkersOf(items.getItemType(item)) == actors.drink_getFluidType(m_actor.getIndex()));
	MustEat& mustEat = *area.getActors().m_mustEat[m_actor.getIndex()].get();
	Quantity quantityDesired = Quantity::create(std::ceil((float)mustEat.getMassFoodRequested().get() / (float)items.getSingleUnitMass(item).get()));
	Quantity quantityEaten = std::min(quantityDesired, items.getQuantity(item));
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), items.getSingleUnitMass(item) * quantityEaten);
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
	MustEat& mustEat = *area.getActors().m_mustEat[m_actor.getIndex()].get();
	Mass massEaten = std::min(actors.getMass(actor), mustEat.getMassFoodRequested());
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	actors.removeMassFromCorpse(actor, massEaten);
}
void EatEvent::eatPlantLeaves(Area& area, PlantIndex plant)
{
	Plants& plants = area.getPlants();
	MustEat& mustEat = *area.getActors().m_mustEat[m_actor.getIndex()].get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), plants.getFoliageMass(plant));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	plants.removeFoliageMass(plant, massEaten);
}
void EatEvent::eatFruitFromPlant(Area& area, PlantIndex plant)
{
	Plants& plants = area.getPlants();
	MustEat& mustEat = *area.getActors().m_mustEat[m_actor.getIndex()].get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), plants.getFruitMass(plant));
	static MaterialTypeId fruitType = MaterialType::byName("fruit");
	Mass unitMass = ItemType::getVolume(PlantSpecies::getFruitItemType(plants.getSpecies(plant))) * MaterialType::getDensity(fruitType);
	Quantity quantityEaten = Quantity::create((massEaten / unitMass).get());
	assert(quantityEaten != 0);
	mustEat.eat(area, unitMass * quantityEaten);
	plants.removeFruitQuantity(plant, quantityEaten);
}
// Path request.
EatPathRequest::EatPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_eatObjective(static_cast<EatObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{
	nlohmann::from_json(data, *this);
}
EatPathRequest::EatPathRequest(Area& area, EatObjective& eo, ActorIndex actor) : m_eatObjective(eo)
{
	assert(m_eatObjective.m_destination.empty());
	Blocks& blocks = area.getBlocks();
	MustEat& mustEat = *area.getActors().m_mustEat[actor].get();
	std::function<bool(BlockIndex)> predicate = nullptr;
	if(m_eatObjective.m_tryToHunt)
	{
		predicate = [&](BlockIndex block)
		{
			for(ActorIndex actor : blocks.actor_getAll(block))
				if(mustEat.canEatActor(area, actor))
				{
					m_huntResult.setTarget(area.getActors().getReferenceTarget(actor));
					return true;
				}
			return false;
		};
	}
	else
	{
		// initalize candidates with null values.
		m_candidates.fill(BlockIndex::null());
		predicate = [&mustEat, this, &area](BlockIndex block)
		{
			uint32_t eatDesire = mustEat.getDesireToEatSomethingAt(area, block);
			if(eatDesire == UINT32_MAX)
				return true;
			if(eatDesire < mustEat.getMinimumAcceptableDesire(area))
				return false;
			if(eatDesire != 0 && m_candidates[eatDesire - 1u].empty())
				m_candidates[eatDesire - 1u] = block;
			return false;
		};
	}
	//TODO: maxRange.
	bool unreserved = false;
	bool reserve = false;
	if(area.getActors().getFaction(actor).exists())
		unreserved = reserve = true;
	createGoAdjacentToCondition(area, actor, predicate, m_eatObjective.m_detour, unreserved, DistanceInBlocks::null(), BlockIndex::null());
}
void EatPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	AnimalSpeciesId species = actors.getSpecies(actor);
	if(m_eatObjective.m_tryToHunt)
	{
		if(!m_huntResult.exists())
		{
			m_eatObjective.m_noFoodFound = true;
			m_eatObjective.execute(area, actor);
			return;
		}
		std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(m_huntResult);
		actors.m_hasObjectives[actor]->addNeed(area, std::move(killObjective));
	}
	else
	{
		if(result.path.empty() && !result.useCurrentPosition)
		{
			if(AnimalSpecies::getEatsMeat(species))
				m_eatObjective.m_tryToHunt = true;
			else
				m_eatObjective.m_noFoodFound = true;
			m_eatObjective.execute(area, actor);
			return;
		}
		FactionId faction = actors.getFactionId(actor);
		if(faction.exists())
		{
			if(result.useCurrentPosition)
			{
				if(!actors.move_tryToReserveOccupied(actor))
				{
					m_eatObjective.execute(area, actor);
					return;
				}
			}
			else if(!actors.move_tryToReserveProposedDestination(actor, result.path))
			{
				m_eatObjective.reset(area, actor);
				m_eatObjective.execute(area, actor);
				return;
			}
		}
		m_eatObjective.m_destination = result.path.back();
		actors.move_setPath(actor, result.path);
	}
}
Json EatPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_eatObjective;
	output["huntTarget"] = m_huntResult;
	return output;
}
// Objective.
EatObjective::EatObjective(Area& area) : Objective(Config::eatPriority), m_eatEvent(area.m_eventSchedule) { }
EatObjective::EatObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, ActorIndex actor) : 
	Objective(data, deserializationMemo), 
	m_eatEvent(deserializationMemo.m_simulation.m_eventSchedule), 
	m_noFoodFound(data["noFoodFound"].get<bool>())
{
	if(data.contains("destination"))
		m_destination = data["destination"].get<BlockIndex>();
	if(data.contains("eventStart"))
		m_eatEvent.schedule(area, Config::stepsToEat, *this, actor, data["eventStart"].get<Step>());
}
Json EatObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noFoodFound"] = m_noFoodFound;
	if(m_destination.exists())
		data["destination"] = m_destination;
	if(m_eatEvent.exists())
		data["eatStart"] = m_eatEvent.getStartStep();
	return data;
}
void EatObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	MustEat& mustEat = *area.getActors().m_mustEat[actor].get();
	// TODO: consider raising minimum desire level at which the actor tries to leave the area.
	if(m_noFoodFound && mustEat.getMinimumAcceptableDesire(area) == 0)
	{
		// We have determined that there is no food here and have attempted to path to the edge of the area so we can leave.
		if(actors.predicateForAnyOccupiedBlock(actor, [&area](BlockIndex block){ return area.getBlocks().isEdge(block); }))
			// We are at the edge and can leave.
			actors.leaveArea(actor);
		else
			// No food and no escape, or not yet hungry enough for what is avalible.
			actors.objective_canNotFulfillNeed(actor, *this);
		return;
	}
	BlockIndex adjacent = mustEat.getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability(area);
	if(m_destination.empty())
	{
		if(adjacent.empty())
			// Find destination.
			makePathRequest(area, actor);
		else
		{
			m_destination = actors.getLocation(actor);
			// Start eating.
			// TODO: reserve occupied?
			m_eatEvent.schedule(area, Config::stepsToEat, *this, actor);
		}
	}
	else
	{	
		if(actors.getLocation(actor) == m_destination)
		{
			if(adjacent.empty())
			{
				// We are at the previously selected location but there is no  longer any food here, try again.
				m_destination.clear();
				actors.canReserve_clearAll(actor);
				makePathRequest(area, actor);
			}
			else
				// Start eating.
				m_eatEvent.schedule(area, Config::stepsToEat, *this, actor);
		}
		else
			actors.move_setDestination(actor, m_destination, m_detour);
	}
}
void EatObjective::cancel(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	m_eatEvent.maybeUnschedule();
	actors.m_mustEat[actor]->m_eatObjective = nullptr;
}
void EatObjective::delay(Area& area, ActorIndex actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
	m_eatEvent.maybeUnschedule();
}
void EatObjective::reset(Area& area, ActorIndex actor)
{
	delay(area, actor);
	m_destination.clear();
	m_noFoodFound = false;
	area.getActors().canReserve_clearAll(actor);
}
void EatObjective::makePathRequest(Area& area, ActorIndex actor)
{
	std::unique_ptr<PathRequest> request = std::make_unique<EatPathRequest>(area, *this, actor);
	area.getActors().move_pathRequestRecord(actor, std::move(request));
}
void EatObjective::noFoodFound()
{
	m_noFoodFound = true;
}
bool EatObjective::canEatAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
	{
		if(actors.eat_canEatItem(actor, item))
			return true;
		if(ItemType::getInternalVolume(items.getItemType(item)) != 0)
			for(ItemIndex i : items.cargo_getItems(item))
				if(actors.eat_canEatItem(actor, i))
					return true;
	}
	AnimalSpeciesId species = actors.getSpecies(actor);
	if(AnimalSpecies::getEatsMeat(species))
		for(ActorIndex actor : blocks.actor_getAll(block))
			if(!actors.isAlive(actor) && AnimalSpecies::getFluidType(species) == AnimalSpecies::getFluidType(actors.getSpecies(actor)))
				return true;
	if(blocks.plant_exists(block))
	{
		const PlantIndex plant = blocks.plant_get(block);
		if(AnimalSpecies::getEatsFruit(species) && PlantSpecies::getFluidType(area.getPlants().getSpecies(plant)) == AnimalSpecies::getFluidType(species))
			if(actors.eat_canEatPlant(actor, blocks.plant_get(block)))
				return true;
	}
	return false;
}
