#include "eat.h"
#include "actor.h"
#include "block.h"
#include "plant.h"
#include "area.h"
#include "kill.h"
#include "path.h"
#include "util.h"

EatEvent::EatEvent(const Step delay, EatObjective& eo) : ScheduledEventWithPercent(eo.m_actor.getSimulation(), delay), m_eatObjective(eo) { }

void EatEvent::execute()
{
	Block* blockContainingFood = getBlockWithMostDesiredFoodInReach();
	auto& actor = m_eatObjective.m_actor;
	if(blockContainingFood == nullptr)
	{
		actor.m_hasObjectives.cannotFulfillObjective(m_eatObjective);
		return;
	}
	for(Item* item : blockContainingFood->m_hasItems.getAll())
		if(item->isPreparedMeal())
		{
			eatPreparedMeal(*item);
			return;
		}
	if(actor.m_species.eatsMeat)
		for(Actor* actor : blockContainingFood->m_hasActors.getAll())
			if(!actor->m_alive && m_eatObjective.m_actor.m_mustEat.canEat(*actor))
			{
				eatActor(*actor);
				return;
			}
	if(blockContainingFood->m_hasPlant.exists())
	{
		auto& plant = blockContainingFood->m_hasPlant.get();
		if(actor.m_species.eatsFruit && plant.m_plantSpecies.fluidType == actor.m_species.fluidType && plant.getFruitMass() != 0)
			eatFruitFromPlant(plant);
		else if(actor.m_species.eatsLeaves && plant.m_plantSpecies.fluidType == actor.m_species.fluidType && plant.getFoliageMass() != 0)
			eatPlantLeaves(plant);
	}

}
void EatEvent::clearReferences() { m_eatObjective.m_eatEvent.clearPointer(); }
Block* EatEvent::getBlockWithMostDesiredFoodInReach() const
{
	Block* output = nullptr;
	uint32_t highestDesirability = 0;
	for(Block* block : m_eatObjective.m_actor.getOccupiedAndAdjacent())
	{
		uint32_t blockDesirability = m_eatObjective.m_actor.m_mustEat.getDesireToEatSomethingAt(*block);
		if(blockDesirability == UINT32_MAX)
			return block;
		if(blockDesirability >= m_eatObjective.m_actor.m_mustEat.getMinimumAcceptableDesire() && blockDesirability > highestDesirability)
		{
			output = block;
			highestDesirability = blockDesirability;
		}
	}
	return output;
}
void EatEvent::eatPreparedMeal(Item& item)
{
	assert(m_eatObjective.m_actor.m_mustEat.canEat(item));
	assert(item.isPreparedMeal());
	auto& eater = m_eatObjective.m_actor;
	uint32_t massEaten = item.m_mass;
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	item.destroy();
	if(!eater.m_mustEat.needsFood())
		eater.m_hasObjectives.objectiveComplete(m_eatObjective);
}
void EatEvent::eatGenericItem(Item& item)
{
	assert(item.m_itemType.edibleForDrinkersOf == &m_eatObjective.m_actor.m_mustDrink.getFluidType());
	auto& eater = m_eatObjective.m_actor;
	uint32_t quantityDesired = eater.m_mustEat.getMassFoodRequested() / item.singleUnitMass();
	uint32_t quantityEaten = std::min(quantityDesired, item.m_quantity);
	uint32_t massEaten = quantityEaten * item.singleUnitMass();
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	item.m_quantity -= quantityEaten;
	if(item.m_quantity == 0)
		item.destroy();
	if(eater.m_mustEat.getMassFoodRequested() == 0)
		eater.m_hasObjectives.objectiveComplete(m_eatObjective);
}
void EatEvent::eatActor(Actor& actor)
{
	assert(!actor.m_alive);
	assert(actor.getMass() != 0);
	auto& eater = m_eatObjective.m_actor;
	uint32_t massEaten = std::min(actor.getMass(), eater.m_mustEat.getMassFoodRequested());
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	actor.removeMassFromCorpse(massEaten);
	if(eater.m_mustEat.getMassFoodRequested() == 0)
		eater.m_hasObjectives.objectiveComplete(m_eatObjective);
}
void EatEvent::eatPlantLeaves(Plant& plant)
{
	auto& eater = m_eatObjective.m_actor;
	uint32_t massEaten = std::min(eater.m_mustEat.getMassFoodRequested(), plant.getFoliageMass());
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	plant.removeFoliageMass(massEaten);
	if(eater.m_mustEat.getMassFoodRequested() == 0)
		eater.m_hasObjectives.objectiveComplete(m_eatObjective);
}
void EatEvent::eatFruitFromPlant(Plant& plant)
{
	auto& eater = m_eatObjective.m_actor;
	uint32_t massEaten = std::min(eater.m_mustEat.getMassFoodRequested(), plant.getFruitMass());
	static const MaterialType& fruitType = MaterialType::byName("fruit");
	uint32_t unitMass = plant.m_plantSpecies.harvestData->fruitItemType.volume * fruitType.density;
	uint32_t quantityEaten = massEaten / unitMass;
	assert(quantityEaten != 0);
	eater.m_mustEat.eat(quantityEaten * unitMass);
	plant.removeFruitQuantity(quantityEaten);
	if(eater.m_mustEat.getMassFoodRequested() == 0)
		eater.m_hasObjectives.objectiveComplete(m_eatObjective);
}
HungerEvent::HungerEvent(const Step delay, Actor& a) : ScheduledEventWithPercent(a.getSimulation(), delay), m_actor(a) { }
void HungerEvent::execute()
{
	m_actor.m_mustEat.setNeedsFood();
}
void HungerEvent::clearReferences() { m_actor.m_mustEat.m_hungerEvent.clearPointer(); }
EatThreadedTask::EatThreadedTask(EatObjective& eo) : PathToBlockBaseThreadedTask(eo.m_actor.getThreadedTaskEngine()), m_eatObjective(eo), m_huntResult(nullptr) {}
void EatThreadedTask::readStep()
{
	if(m_eatObjective.m_foodLocation == nullptr)
	{
		constexpr uint32_t maxRankedEatDesire = 3;
		std::array<Block*, maxRankedEatDesire> candidates = {};
		auto destinationCondition = [&](Block& block)
		{
			uint32_t eatDesire = m_eatObjective.m_actor.m_mustEat.getDesireToEatSomethingAt(block);
			if(eatDesire == UINT32_MAX)
				return true;
			if(eatDesire < m_eatObjective.m_actor.m_mustEat.getMinimumAcceptableDesire())
				return false;
			if(candidates[eatDesire - 1u] == nullptr)
				candidates[eatDesire - 1u] = &block;
			return false;
		};
		m_route = path::getForActorToPredicate(m_eatObjective.m_actor, destinationCondition, Config::maxBlocksToLookForBetterFood);
		if(m_route.empty())
		{
			for(size_t i = maxRankedEatDesire; i != 0; --i)
				if(candidates[i - 1] != nullptr)
				{
					pathTo(m_eatObjective.m_actor, *candidates[i - 1]);
					return;
				}
			// Nothing to scavenge or graze, maybe hunt.
			if(m_eatObjective.m_actor.m_species.eatsMeat)
			{
				m_huntResult = nullptr;
				auto huntCondition = [&](Block& block)
				{
					for(Actor* actor : block.m_hasActors.getAll())
						if(m_eatObjective.m_actor.m_mustEat.canEat(*actor))
						{
							m_huntResult = actor;
							return true;
						}
					return false;
				};
				// discard the location result, we are already collecting the actor result.
				path::getForActorToPredicateReturnEndOnly(m_eatObjective.m_actor, huntCondition);
			}
		}
	} 
	else
	{
		assert(m_eatObjective.m_eatingLocation == nullptr);
		auto eatingLocationCondition = [&](Block& block)
		{
			if(block.m_reservable.isFullyReserved(*m_eatObjective.m_actor.getFaction()))
				return false;
			static const ItemType& chair = ItemType::byName("chair");
			if(!block.m_hasItems.hasInstalledItemType(chair))
				return false;
			for(Block* adjacent : block.m_adjacentsVector)
			{
				if(adjacent->isSolid())
					continue;
				static const ItemType& table = ItemType::byName("table");
				if(adjacent->m_hasItems.hasInstalledItemType(table))
					return true;
			}
			return false;
		};
		std::vector<Block*> path = path::getForActorToPredicate(m_eatObjective.m_actor, eatingLocationCondition, Config::maxDistanceToLookForEatingLocation);
		if(path.empty())
			m_eatObjective.m_noEatingLocationFound = true;
		else
		{
			m_eatObjective.m_actor.m_canMove.setPath(path);
			m_eatObjective.m_eatingLocation = path.back();
		}
	}
}
void EatThreadedTask::writeStep()
{
	if(m_route.empty())
	{
		if(m_huntResult == nullptr)
			m_eatObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_eatObjective);
		else
		{
			std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(m_eatObjective.m_actor, *m_huntResult);
			m_eatObjective.m_actor.m_hasObjectives.addNeed(std::move(killObjective));
		}
	}
	else
	{
		m_eatObjective.m_actor.m_canMove.setPath(m_route);
		//TODO: reserve food if sentient.
	}
	cacheMoveCosts(m_eatObjective.m_actor);
}
void EatThreadedTask::clearReferences() { m_eatObjective.m_threadedTask.clearPointer(); }
EatObjective::EatObjective(Actor& a) :
	Objective(Config::eatPriority), m_actor(a), m_threadedTask(a.getThreadedTaskEngine()), m_eatEvent(a.getEventSchedule()), m_foodLocation(nullptr), m_foodItem(nullptr), m_eatingLocation(m_actor.m_mustEat.m_eatingLocation), m_noEatingLocationFound(false) { }
void EatObjective::execute()
{
	if(m_foodLocation == nullptr)
		// Set foodLocation and foodItem.
		m_threadedTask.create(*this);
	else if(!m_actor.isSentient() || !m_noEatingLocationFound)
	{	
		// Uncivilized eating.
		if(m_actor.m_location == m_foodLocation)
			m_eatEvent.schedule(Config::stepsToEat, *this);
		else
			m_actor.m_canMove.setDestination(*m_foodLocation);
	}
	else
	{
		// Civilized eating.
		if(m_actor.isSentient())
		{
			// Has meal
			if(m_actor.m_canPickup.isCarrying(*m_foodItem))
			{
				if(m_eatingLocation == nullptr)
					// Set eating location.
					m_threadedTask.create(*this);
				else
				{
					// Is at eating location.
					if(m_actor.m_location == m_eatingLocation)
						m_eatEvent.schedule(Config::stepsToEat, *this);
					else
						m_actor.m_canMove.setDestination(*m_eatingLocation);
				}
			}
			else
			{
				// Is at pickup location.
				if(m_actor.m_location == m_foodLocation)
				{
					uint32_t massToPickUp = std::min({m_actor.m_mustEat.getMassFoodRequested(), m_foodItem->m_mass, m_actor.m_canPickup.canPickupQuantityOf(m_foodItem->m_itemType, m_foodItem->m_materialType)});
					uint32_t quantityToPickUp = massToPickUp / m_foodItem->singleUnitMass();
					m_actor.m_canPickup.pickUp(*m_foodItem, quantityToPickUp);
					execute();
				}
				else
					m_actor.m_canMove.setDestination(*m_foodLocation);
			}

		}
	}
}
bool EatObjective::canEatAt(const Block& block) const
{
	for(const Item* item : block.m_hasItems.getAll())
	{
		if(m_actor.m_mustEat.canEat(*item))
			return true;
		if(item->m_itemType.internalVolume != 0)
			for(Item* i : item->m_hasCargo.getItems())
				if(m_actor.m_mustEat.canEat(*i))
					return true;
	}
	if(m_actor.m_species.eatsMeat)
		for(const Actor* actor : block.m_hasActors.getAll())
			if(!actor->m_alive && m_actor.m_species.fluidType == actor->m_species.fluidType)
				return true;
	if(block.m_hasPlant.exists())
	{
		const Plant& plant = block.m_hasPlant.get();
		if(m_actor.m_species.eatsFruit && plant.m_plantSpecies.fluidType == m_actor.m_species.fluidType)
			if(m_actor.m_mustEat.canEat(block.m_hasPlant.get()))
				return true;
	}
	return false;
}
MustEat::MustEat(Actor& a) : m_actor(a), m_massFoodRequested(0), m_hungerEvent(a.getEventSchedule()), m_eatingLocation(nullptr)
{
	m_hungerEvent.schedule(m_actor.m_species.stepsEatFrequency, m_actor);
}
bool MustEat::canEat(const Actor& actor) const
{
	if(actor.m_alive)
		return false;
	if(!m_actor.m_species.eatsMeat)
		return false;
	if(m_actor.m_species.fluidType != actor.m_species.fluidType)
		return false;
	return true;
}
bool MustEat::canEat(const Plant& plant) const
{
	if(m_actor.m_species.eatsLeaves && plant.getPercentFoliage() != 0)
		return true;
	if(m_actor.m_species.eatsFruit && plant.m_quantityToHarvest != 0)
		return true;
	return false;
}
bool MustEat::canEat(const Item& item) const
{
	return item.m_itemType.edibleForDrinkersOf == &m_actor.m_mustDrink.getFluidType();
}
void MustEat::eat(uint32_t mass)
{
	assert(mass <= m_massFoodRequested);
	m_massFoodRequested -= mass;
	m_hungerEvent.unschedule();
	if(m_massFoodRequested == 0)
		m_actor.m_canGrow.maybeStart();
	else
	{
		Step delay = util::scaleByInverseFraction(m_actor.m_species.stepsTillDieWithoutFood, m_massFoodRequested, massFoodForBodyMass());
		m_actor.m_hasObjectives.addNeed(std::make_unique<EatObjective>(m_actor));
		m_actor.m_mustEat.m_hungerEvent.schedule(delay, m_actor);
	}
}
void MustEat::setNeedsFood()
{
	if(m_hungerEvent.exists())
		m_actor.die(CauseOfDeath::hunger);
	else
	{
		m_hungerEvent.schedule(m_actor.m_species.stepsTillDieWithoutFood, m_actor);
		m_actor.m_canGrow.stop();
		m_massFoodRequested = massFoodForBodyMass();
		m_actor.m_hasObjectives.addNeed(std::make_unique<EatObjective>(m_actor));
	}
}
bool MustEat::needsFood() const { return m_massFoodRequested != 0; }
uint32_t MustEat::massFoodForBodyMass() const
{
	return m_actor.getMass() / Config::foodRequestedMassPerUnitOfBodyMass;
}
const uint32_t& MustEat::getMassFoodRequested() const { return m_massFoodRequested; }
uint32_t MustEat::getPercentStarved() const
{
	if(!m_hungerEvent.exists())
		return 0;
	return m_hungerEvent.percentComplete();
}
uint32_t MustEat::getDesireToEatSomethingAt(Block& block) const
{
	for(Item* item : block.m_hasItems.getAll())
		if(item->isPreparedMeal())
			return UINT32_MAX;
	if(m_actor.m_species.eatsFruit && block.m_hasPlant.exists())
	{
		Plant& plant = block.m_hasPlant.get();
		if(plant.getFruitMass() != 0)
			return 1;
	}
	if(m_actor.m_species.eatsMeat)
		for(Actor* actor : block.m_hasActors.getAll())
			if(!actor->m_alive && actor->m_species.fluidType == m_actor.m_species.fluidType)
				return 2;
	if(m_actor.m_species.eatsLeaves && block.m_hasPlant.exists())
		if(block.m_hasPlant.get().m_percentFoliage != 0)
			return 3;
	return 0;
}
uint32_t MustEat::getMinimumAcceptableDesire() const
{
	assert(m_hungerEvent.exists());
	return m_hungerEvent.percentComplete() * (3 - Config::percentHungerAcceptableDesireModifier);
}
