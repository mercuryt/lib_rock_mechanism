#include "eat.h"
#include "../area.h"
#include "../blocks/blocks.h"
#include "../items/items.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../itemType.h"
#include "../animalSpecies.h"
#include "../terrainFacade.hpp"
#include "../types.h"
#include "kill.h"

EatEvent::EatEvent(Area& area, const Step& delay, EatObjective& eo, const ActorIndex& actor, const Step start) : ScheduledEvent(area.m_simulation, delay, start), m_eatObjective(eo)
{
	assert(area.getActors().eat_hasObjective(actor));
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void EatEvent::execute(Simulation&, Area *area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	// TODO: use the version of this method on mustEat and delete this one.
	BlockIndex blockContainingFood = actors.eat_getAdjacentBlockWithTheMostDesiredFood(actor);
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
void EatEvent::clearReferences(Simulation &, Area *) { m_eatObjective.m_eatEvent.clearPointer(); }
void EatEvent::eatPreparedMeal(Area &area, const ItemIndex &item)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	assert(actors.eat_canEatItem(actor, item));
	assert(items.isPreparedMeal(item));
	MustEat &mustEat = *area.getActors().m_mustEat[actor].get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), items.getMass(item));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	items.destroy(item);
}
void EatEvent::eatGenericItem(Area &area, const ItemIndex &item)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	assert(ItemType::getEdibleForDrinkersOf(items.getItemType(item)) == actors.drink_getFluidType(actor));
	MustEat &mustEat = *area.getActors().m_mustEat[actor].get();
	assert(mustEat.hasObjective());
	Quantity quantityDesired = Quantity::create(std::ceil((float)mustEat.getMassFoodRequested().get() / (float)items.getSingleUnitMass(item).get()));
	Quantity quantityEaten = std::min(quantityDesired, items.getQuantity(item));
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), items.getSingleUnitMass(item) * quantityEaten);
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	items.removeQuantity(item, quantityEaten);
	if(items.getQuantity(item) == 0)
		items.destroy(item);
}
void EatEvent::eatActor(Area &area, const ActorIndex &actor)
{
	Actors &actors = area.getActors();
	assert(!actors.isAlive(actor));
	assert(actors.getMass(actor) != 0);
	MustEat &mustEat = *area.getActors().m_mustEat[m_actor.getIndex(actors.m_referenceData)].get();
	Mass massEaten = std::min(actors.getMass(actor), mustEat.getMassFoodRequested());
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	actors.removeMassFromCorpse(actor, massEaten);
}
void EatEvent::eatPlantLeaves(Area &area, const PlantIndex &plant)
{
	Plants &plants = area.getPlants();
	Actors &actors = area.getActors();
	MustEat &mustEat = *actors.m_mustEat[m_actor.getIndex(actors.m_referenceData)].get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), plants.getFoliageMass(plant));
	assert(massEaten != 0);
	mustEat.eat(area, massEaten);
	plants.removeFoliageMass(plant, massEaten);
}
void EatEvent::eatFruitFromPlant(Area &area, const PlantIndex &plant)
{
	Plants &plants = area.getPlants();
	Actors &actors = area.getActors();
	MustEat &mustEat = *actors.m_mustEat[m_actor.getIndex(actors.m_referenceData)].get();
	Mass massEaten = std::min(mustEat.getMassFoodRequested(), plants.getFruitMass(plant));
	static MaterialTypeId fruitType = MaterialType::byName(L"fruit");
	Mass unitMass = ItemType::getVolume(PlantSpecies::getFruitItemType(plants.getSpecies(plant))) * MaterialType::getDensity(fruitType);
	Quantity quantityEaten = Quantity::create((massEaten / unitMass).get());
	assert(quantityEaten != 0);
	mustEat.eat(area, unitMass * quantityEaten);
	plants.removeFruitQuantity(plant, quantityEaten);
}
// Path request.
EatPathRequest::EatPathRequest(const Json &data, Area& area, DeserializationMemo &deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_eatObjective(static_cast<EatObjective &>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{ }
EatPathRequest::EatPathRequest(Area &area, EatObjective &eo, const ActorIndex &actorIndex) :
	m_eatObjective(eo)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = DistanceInBlocks::max();
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_eatObjective.m_detour;
	adjacent = true;
}
FindPathResult EatPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	assert(m_eatObjective.m_location.empty());
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	MustEat& mustEat = *area.getActors().m_mustEat[actorIndex].get();
	if(m_eatObjective.m_tryToHunt)
	{
		auto destinationCondition = [&](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
		{
			for(ActorIndex prey : blocks.actor_getAll(block))
				if(mustEat.canEatActor(area, prey))
				{
					m_huntResult.setIndex(prey, area.getActors().m_referenceData);
					return {true, block};
				}
			return {false, block};
		};
		constexpr bool useAnyOccupiedBlock = true;
		return terrainFacade.findPathToConditionBreadthFirst<useAnyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, m_eatObjective.m_detour, adjacent);
	}
	else if(m_eatObjective.m_noFoodFound)
		return terrainFacade.findPathToEdge(memo, start, facing, shape, m_eatObjective.m_detour);
	else
	{
		// initalize candidates with null values.
		m_candidates.fill(BlockIndex::null());
		if(area.getActors().isSentient(actorIndex))
		{
			// Sentients will only return true if the come across a maxRankedDesire (prepared meal).
			// Otherwise they will store candidates ranked by desire.
			// EatPathRequest::callback may use one of those candidates, if the actorIndex is hungry enough.
			auto minimum = mustEat.getMinimumAcceptableDesire(area);
			auto destinationCondition = [&mustEat, this, &area, minimum](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
			{
				uint32_t eatDesire = mustEat.getDesireToEatSomethingAt(area, block);
				if(eatDesire < minimum)
					return {false, block};
				if(eatDesire == maxRankedEatDesire)
					return {true, block};
				if(eatDesire != 0 && m_candidates[eatDesire - 1u].empty())
					m_candidates[eatDesire - 1u] = block;
				return {false, block};
			};
			constexpr bool useAnyOccupiedBlock = true;
			return terrainFacade.findPathToConditionBreadthFirst<useAnyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, m_eatObjective.m_detour, adjacent);
		}
		else
		{
			// Nonsentients will eat whatever they come across first.
			// Having preference would be nice but this is better for performance.
			auto destinationCondition = [&mustEat, &area](const BlockIndex& block, const Facing&) -> std::pair<bool, BlockIndex>
			{
				return {mustEat.getDesireToEatSomethingAt(area, block) != 0, block};
			};
			constexpr bool useAnyOccupiedBlock = true;
			return terrainFacade.findPathToConditionBreadthFirst<useAnyOccupiedBlock, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, m_eatObjective.m_detour, adjacent);
		}
	}
}
void EatPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors &actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	AnimalSpeciesId species = actors.getSpecies(actorIndex);
	if(m_eatObjective.m_tryToHunt)
	{
		if(!m_huntResult.exists())
		{
			// Nothing found to hunt.
			m_eatObjective.m_noFoodFound = true;
			m_eatObjective.execute(area, actorIndex);
		}
		else
		{
			// Hunt target found, create kill objective.
			actors.m_hasObjectives[actorIndex]->addNeed(area, std::make_unique<KillObjective>(m_huntResult));
		}
	}
	else if(m_eatObjective.m_noFoodFound)
	{
		// Result of trying to leave area.
		assert(actors.eat_getMinimumAcceptableDesire(actorIndex) == 0);
		if(actors.isOnEdge(actorIndex))
		{
			// Leave area.
			assert(result.useCurrentPosition);
			m_eatObjective.execute(area, actorIndex);
		}
		else if(result.path.empty())
		{
			// No path to edge, cannot fulfill.
			assert(!result.useCurrentPosition);
			m_eatObjective.m_noFoodFound = m_eatObjective.m_tryToHunt = false;
			m_eatObjective.m_location.clear();
			actors.objective_canNotFulfillNeed(actorIndex, m_eatObjective);
		}
		else
			// Path to edge found.
			actors.move_setPath(actorIndex, result.path);
	}
	else
	{
		if(result.path.empty() && !result.useCurrentPosition)
		{
			if(AnimalSpecies::getEatsMeat(species) && (!actors.isSentient(actorIndex) || actors.eat_getPercentStarved(actorIndex) > Config::minimumHungerLevelThresholds[1]))
			{
				m_eatObjective.m_tryToHunt = true;
				m_eatObjective.execute(area, actorIndex);
			}
			else
			{
				// We didn't find anything with max desirability, look over what we did find and choose the best.
				// candidates are sorted low to high, so iterate backwards untill we find a slot which is set.
				for(auto iter = m_candidates.rbegin(); iter != m_candidates.rend(); ++iter)
					if(iter->exists())
					{
						m_eatObjective.m_location = *iter;
						break;
					}
				if(m_eatObjective.m_location.empty())
				{
					// No candidates found, either supress need or try to leave area.
					m_eatObjective.m_noFoodFound = true;
					m_eatObjective.execute(area, actorIndex);
					return;
				}
				m_eatObjective.execute(area, actorIndex);
			}
		}
		else
		{
			// Found max desirability target, use the result path.
			m_eatObjective.m_location = result.blockThatPassedPredicate;
			if(result.useCurrentPosition)
				m_eatObjective.execute(area, actorIndex);
			else
				// TODO: move rather then copy path.
				actors.move_setPath(actorIndex, result.path);
		}
	}
}
Json EatPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_eatObjective;
	output["huntTarget"] = m_huntResult;
	output["type"] = "eat";
	return output;
}
// Objective.
EatObjective::EatObjective(Area &area) :
	Objective(Config::eatPriority),
	m_eatEvent(area.m_eventSchedule)
{}
EatObjective::EatObjective(const Json &data, DeserializationMemo &deserializationMemo, Area &area, const ActorIndex &actor) :
	Objective(data, deserializationMemo),
	m_eatEvent(deserializationMemo.m_simulation.m_eventSchedule),
	m_noFoodFound(data["noFoodFound"].get<bool>())
{
	if(data.contains("destination"))
		m_location = data["location"].get<BlockIndex>();
	if(data.contains("eventStart"))
		m_eatEvent.schedule(area, Config::stepsToEat, *this, actor, data["eventStart"].get<Step>());
}
Json EatObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noFoodFound"] = m_noFoodFound;
	if(m_location.exists())
		data["location"] = m_location;
	if(m_eatEvent.exists())
		data["eatStart"] = m_eatEvent.getStartStep();
	return data;
}
void EatObjective::execute(Area &area, const ActorIndex &actor)
{
	Actors &actors = area.getActors();
	MustEat &mustEat = *area.getActors().m_mustEat[actor].get();
	// If the objective was supressed and then unsupressed MustEat::m_objective needs to be restored.
	if(!mustEat.hasObjective())
		mustEat.setObjective(*this);
	// TODO: consider raising minimum desire level at which the actor tries to leave the area.
	if(m_noFoodFound)
	{
		if(mustEat.getMinimumAcceptableDesire(area) == 0)
		{
			// Actor is hungry enough to leave the area.
			if(actors.isOnEdge(actor))
				// Actor is at the edge, leave.
				actors.leaveArea(actor);
			else
				// Path to edge
				makePathRequest(area, actor);
		}
		else
		{
			// Not hungry enough to leave yet.
			m_noFoodFound = m_tryToHunt = false;
			m_location.clear();
			actors.objective_canNotFulfillNeed(actor, *this);
		}
		return;
	}
	if(m_location.empty())
	{
		BlockIndex adjacent = mustEat.getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability(area);
		if(adjacent.empty())
			// Find destination.
			makePathRequest(area, actor);
		else
		{
			m_location = actors.getLocation(actor);
			// Start eating.
			// TODO: reserve occupied?
			m_eatEvent.schedule(area, Config::stepsToEat, *this, actor);
		}
	}
	else
	{
		if(actors.isAdjacentToLocation(actor, m_location))
		{
			BlockIndex adjacent = mustEat.getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability(area);
			if(adjacent.empty())
			{
				// We are at the previously selected location but there is no  longer any food here, try again.
				m_location.clear();
				actors.canReserve_clearAll(actor);
				makePathRequest(area, actor);
			}
			else
				// Start eating.
				m_eatEvent.schedule(area, Config::stepsToEat, *this, actor);
		}
		else
		{
			bool adjacent = true;
			bool unreserved = false;
			bool reserve = false;
			actors.move_setDestination(actor, m_location, m_detour, adjacent, unreserved, reserve);
		}
	}
}
void EatObjective::cancel(Area &area, const ActorIndex &actor)
{
	Actors &actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	m_eatEvent.maybeUnschedule();
	actors.m_mustEat[actor]->m_eatObjective = nullptr;
}
void EatObjective::delay(Area &area, const ActorIndex &actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
	m_eatEvent.maybeUnschedule();
}
void EatObjective::reset(Area &area, const ActorIndex &actor)
{
	delay(area, actor);
	m_location.clear();
	m_noFoodFound = false;
	m_tryToHunt = false;
	area.getActors().canReserve_clearAll(actor);
}
void EatObjective::makePathRequest(Area &area, const ActorIndex &actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<EatPathRequest>(area, *this, actor));
}
void EatObjective::noFoodFound()
{
	m_noFoodFound = true;
}
bool EatObjective::canEatAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const
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