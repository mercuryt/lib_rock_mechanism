#include "eat.h"
#include "actor.h"
#include "block.h"
#include "config.h"
#include "objective.h"
#include "plant.h"
#include "area.h"
#include "kill.h"
#include "util.h"
#include "simulation.h"

EatEvent::EatEvent(const Step delay, EatObjective& eo, const Step start) : ScheduledEvent(eo.m_actor.getSimulation(), delay, start), m_eatObjective(eo) { }

void EatEvent::execute()
{
	Block* blockContainingFood = getBlockWithMostDesiredFoodInReach();
	auto& actor = m_eatObjective.m_actor;
	if(blockContainingFood == nullptr)
	{
		actor.m_hasObjectives.cannotCompleteSubobjective();
		return;
	}
	for(Item* item : blockContainingFood->m_hasItems.getAll())
	{
		if(item->isPreparedMeal())
		{
			eatPreparedMeal(*item);
			return;
		}
		if(item->m_itemType.edibleForDrinkersOf == &actor.m_species.fluidType)
		{
			eatGenericItem(*item);
			return;
		}
	}
	if(actor.m_species.eatsMeat)
		for(Actor* actor : blockContainingFood->m_hasActors.getAll())
			if(!actor->isAlive() && m_eatObjective.m_actor.m_mustEat.canEat(*actor))
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
	Block* found = nullptr;
	uint32_t highestDesirability = 0;
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		uint32_t blockDesirability = m_eatObjective.m_actor.m_mustEat.getDesireToEatSomethingAt(block);
		if(blockDesirability == UINT32_MAX)
			return true;
		if(blockDesirability >= m_eatObjective.m_actor.m_mustEat.getMinimumAcceptableDesire() && blockDesirability > highestDesirability)
		{
			found = const_cast<Block*>(&block);
			highestDesirability = blockDesirability;
		}
		return false;
	};
	Block* output = m_eatObjective.m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
	if(output == nullptr)
	       output = found;	
	return output;
}
void EatEvent::eatPreparedMeal(Item& item)
{
	assert(m_eatObjective.m_actor.m_mustEat.canEat(item));
	assert(item.isPreparedMeal());
	auto& eater = m_eatObjective.m_actor;
	Mass massEaten = std::min(eater.m_mustEat.getMassFoodRequested(), item.m_mass);
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	item.destroy();
}
void EatEvent::eatGenericItem(Item& item)
{
	assert(item.m_itemType.edibleForDrinkersOf == &m_eatObjective.m_actor.m_mustDrink.getFluidType());
	auto& eater = m_eatObjective.m_actor;
	uint32_t quantityDesired = std::ceil((float)eater.m_mustEat.getMassFoodRequested() / (float)item.singleUnitMass());
	uint32_t quantityEaten = std::min(quantityDesired, item.getQuantity());
	Mass massEaten = std::min(eater.m_mustEat.getMassFoodRequested(), quantityEaten * item.singleUnitMass());
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	item.removeQuantity(quantityEaten);
	if(item.getQuantity() == 0)
		item.destroy();
}
void EatEvent::eatActor(Actor& actor)
{
	assert(!actor.isAlive());
	assert(actor.getMass() != 0);
	auto& eater = m_eatObjective.m_actor;
	Mass massEaten = std::min(actor.getMass(), eater.m_mustEat.getMassFoodRequested());
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	actor.removeMassFromCorpse(massEaten);
}
void EatEvent::eatPlantLeaves(Plant& plant)
{
	auto& eater = m_eatObjective.m_actor;
	Mass massEaten = std::min(eater.m_mustEat.getMassFoodRequested(), plant.getFoliageMass());
	assert(massEaten != 0);
	eater.m_mustEat.eat(massEaten);
	plant.removeFoliageMass(massEaten);
}
void EatEvent::eatFruitFromPlant(Plant& plant)
{
	auto& eater = m_eatObjective.m_actor;
	Mass massEaten = std::min(eater.m_mustEat.getMassFoodRequested(), plant.getFruitMass());
	static const MaterialType& fruitType = MaterialType::byName("fruit");
	uint32_t unitMass = plant.m_plantSpecies.harvestData->fruitItemType.volume * fruitType.density;
	uint32_t quantityEaten = massEaten / unitMass;
	assert(quantityEaten != 0);
	eater.m_mustEat.eat(quantityEaten * unitMass);
	plant.removeFruitQuantity(quantityEaten);
}
HungerEvent::HungerEvent(const Step delay, Actor& a, const Step start) : ScheduledEvent(a.getSimulation(), delay, start), m_actor(a) { }
void HungerEvent::execute()
{
	m_actor.m_mustEat.setNeedsFood();
}
void HungerEvent::clearReferences() { m_actor.m_mustEat.m_hungerEvent.clearPointer(); }
EatThreadedTask::EatThreadedTask(EatObjective& eo) : ThreadedTask(eo.m_actor.getThreadedTaskEngine()), m_eatObjective(eo), m_huntResult(nullptr), m_findsPath(eo.m_actor, eo.m_detour), m_noFoodFound(false) {}
void EatThreadedTask::readStep()
{
	assert(m_eatObjective.m_destination == nullptr);
	constexpr uint32_t maxRankedEatDesire = 3;
	std::array<const Block*, maxRankedEatDesire> candidates = {};
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
			uint32_t eatDesire = m_eatObjective.m_actor.m_mustEat.getDesireToEatSomethingAt(block);
			if(eatDesire == UINT32_MAX)
				return true;
			if(eatDesire < m_eatObjective.m_actor.m_mustEat.getMinimumAcceptableDesire())
				return false;
			if(eatDesire != 0 && candidates[eatDesire - 1u] == nullptr)
				candidates[eatDesire - 1u] = &block;
		return false;
	};
	//TODO: maxRange.
	if(m_eatObjective.m_actor.getFaction())
		m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_eatObjective.m_actor.getFaction());
	else
		m_findsPath.pathToAdjacentToPredicate(predicate);
	if(m_findsPath.found())
		m_eatObjective.m_destination = m_findsPath.getPath().back();
	else if (m_findsPath.m_useCurrentLocation)
		m_eatObjective.m_destination = m_eatObjective.m_actor.m_location;
	else
	{
		for(size_t i = maxRankedEatDesire; i != 0; --i)
			if(candidates[i - 1] != nullptr)
			{
				m_findsPath.pathAdjacentToBlock(*candidates[i - 1]);
				if(m_findsPath.found())
					m_eatObjective.m_destination = m_findsPath.getPath().back();
				return;
			}
		// Nothing to scavenge or graze, maybe hunt.
		if(m_eatObjective.m_actor.m_species.eatsMeat)
		{
			m_huntResult = nullptr;
			std::function<bool(const Block&)> predicate = [&](const Block& block)
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
			m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_eatObjective.m_actor.getFaction());
		}
		if(m_huntResult == nullptr)
		{
			// Nothing to eat in this area, try to exit.
			m_findsPath.pathToAreaEdge();
			m_noFoodFound = true;
		}
	}
}
void EatThreadedTask::writeStep()
{
	m_findsPath.cacheMoveCosts();
	if(m_noFoodFound)
		m_eatObjective.noFoodFound();
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation)
			m_eatObjective.execute();
		else if(m_huntResult == nullptr)
		{
			// Unable to find food or a path to exit the area.
			m_eatObjective.m_actor.m_hasObjectives.cannotFulfillNeed(m_eatObjective);
		}
		else
		{
			// The hunt is on.
			std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(m_eatObjective.m_actor, *m_huntResult);
			m_eatObjective.m_actor.m_hasObjectives.addNeed(std::move(killObjective));
		}
	}
	else
	{
		m_eatObjective.m_destination = m_findsPath.getPath().back();
		m_eatObjective.m_actor.m_canMove.setPath(m_findsPath.getPath());
	}
}
void EatThreadedTask::clearReferences() { m_eatObjective.m_threadedTask.clearPointer(); }
EatObjective::EatObjective(Actor& a) :
	Objective(a, Config::eatPriority), m_threadedTask(a.getThreadedTaskEngine()), m_eatEvent(a.getEventSchedule()), m_destination(nullptr), m_noFoodFound(false) { }
EatObjective::EatObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), m_eatEvent(deserializationMemo.m_simulation.m_eventSchedule), m_destination(nullptr), m_noFoodFound(data["noFoodFound"].get<bool>())
{
	if(data.contains("destination"))
		m_destination = &deserializationMemo.m_simulation.getBlockForJsonQuery(data["destination"]);
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_eatEvent.schedule(Config::stepsToEat, *this, data["eventStart"].get<Step>());
}
Json EatObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noFoodFound"] = m_noFoodFound;
	if(m_destination)
		data["destination"] = m_destination;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_eatEvent.exists())
		data["eatStart"] = m_eatEvent.getStartStep();
	return data;
}
void EatObjective::execute()
{
	if(m_noFoodFound)
	{
		// We have determined that there is no food here and have attempted to path to the edge of the area so we can leave.
		if(m_actor.predicateForAnyOccupiedBlock([](const Block& block){ return block.m_isEdge; }))
			// We are at the edge and can leave.
			m_actor.leaveArea();
		else
			// No food and no escape.
			m_actor.m_hasObjectives.cannotFulfillNeed(*this);
		return;
	}
	// TODO: getAdjacentDoesn't need no run quite so often, only at start and end of path, not when stopped/detoured in the middle.
	Block* adjacent = m_actor.m_mustEat.getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability();
	if(m_destination == nullptr)
	{
		if(adjacent == nullptr)
			// Find destination.
			m_threadedTask.create(*this);
		else
		{
			m_destination = m_actor.m_location;
			// Start eating.
			m_eatEvent.schedule(Config::stepsToEat, *this);
		}
	}
	else
	{	
		if(m_actor.m_location == m_destination)
		{
			if(adjacent == nullptr)
			{
				// We are at the previously selected location but there is no  longer any food here, try again.
				m_destination = nullptr;
				m_actor.m_canReserve.deleteAllWithoutCallback();
				m_threadedTask.create(*this);
			}
			else
				// Start eating.
				m_eatEvent.schedule(Config::stepsToEat, *this);
		}
		else
			m_actor.m_canMove.setDestination(*m_destination, m_detour);
	}
}
void EatObjective::cancel()
{
	m_threadedTask.maybeCancel();
	m_eatEvent.maybeUnschedule();
	m_actor.m_mustEat.m_eatObjective = nullptr;
}
void EatObjective::delay()
{
	m_threadedTask.maybeCancel();
	m_eatEvent.maybeUnschedule();
}
void EatObjective::reset()
{
	delay();
	m_destination = nullptr;
	m_noFoodFound = false;
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
void EatObjective::noFoodFound()
{
	m_noFoodFound = true;
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
			if(!actor->isAlive() && m_actor.m_species.fluidType == actor->m_species.fluidType)
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
MustEat::MustEat(Actor& a) : m_actor(a), m_massFoodRequested(0), m_hungerEvent(a.getEventSchedule()), m_eatObjective(nullptr), m_eatingLocation(nullptr)
{
	m_hungerEvent.schedule(m_actor.m_species.stepsEatFrequency, m_actor);
}
MustEat::MustEat(const Json& data, Actor& a) : 
	m_actor(a), m_massFoodRequested(data["massFoodRequested"].get<Mass>()),
	m_hungerEvent(a.getEventSchedule()), m_eatObjective(nullptr)
{
	if(data.contains("eatingLocation"))
		m_eatingLocation = &m_actor.getSimulation().getBlockForJsonQuery(data["eatingLocation"]);
	if(data.contains("hungerEventStart"))
		m_hungerEvent.schedule(m_actor.m_species.stepsEatFrequency, m_actor, data["hungerEventStart"].get<Step>());
}
Json MustEat::toJson() const
{
	Json data;
	data["massFoodRequested"] = m_massFoodRequested;
	if(m_eatingLocation)
		data["eatingLocation"] = m_eatingLocation->positionToJson();
	data["hungerEventStart"] = m_hungerEvent.getStartStep();
	return data;
}
bool MustEat::canEat(const Actor& actor) const
{
	if(actor.isAlive())
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
void MustEat::eat(Mass mass)
{
	assert(mass <= m_massFoodRequested);
	assert(mass != 0);
	m_massFoodRequested -= mass;
	m_hungerEvent.unschedule();
	Step stepsToNextHungerEvent;
	if(m_massFoodRequested == 0)
	{
		m_actor.m_canGrow.maybeStart();
		stepsToNextHungerEvent = m_actor.m_species.stepsEatFrequency;
		m_actor.m_hasObjectives.objectiveComplete(*m_eatObjective);
		m_eatObjective = nullptr;
		m_actor.m_mustEat.m_hungerEvent.schedule(stepsToNextHungerEvent, m_actor);
	}
	else
	{
		//TODO: ERROR HERE!
		stepsToNextHungerEvent = util::scaleByInverseFraction(m_actor.m_species.stepsTillDieWithoutFood, m_massFoodRequested, massFoodForBodyMass());
		m_actor.m_mustEat.m_hungerEvent.schedule(stepsToNextHungerEvent, m_actor);
		m_actor.m_hasObjectives.subobjectiveComplete();
	}
}
void MustEat::setNeedsFood()
{
	if(needsFood())
		m_actor.die(CauseOfDeath::hunger);
	else
	{
		m_hungerEvent.schedule(m_actor.m_species.stepsTillDieWithoutFood, m_actor);
		m_actor.m_canGrow.stop();
		m_massFoodRequested = massFoodForBodyMass();
		std::unique_ptr<Objective> objective = std::make_unique<EatObjective>(m_actor);
		m_eatObjective = static_cast<EatObjective*>(objective.get());
		m_actor.m_hasObjectives.addNeed(std::move(objective));
	}
}
void MustEat::onDeath()
{
	m_hungerEvent.maybeUnschedule();
}
bool MustEat::needsFood() const { return m_massFoodRequested != 0; }
uint32_t MustEat::massFoodForBodyMass() const
{
	return std::max(1u, m_actor.getMass() / Config::unitsBodyMassPerUnitFoodConsumed);
}
const uint32_t& MustEat::getMassFoodRequested() const { return m_massFoodRequested; }
Percent MustEat::getPercentStarved() const
{
	if(!m_hungerEvent.exists())
		return 0;
	return m_hungerEvent.percentComplete();
}
uint32_t MustEat::getDesireToEatSomethingAt(const Block& block) const
{
	for(Item* item : block.m_hasItems.getAll())
	{
		if(item->isPreparedMeal())
			return UINT32_MAX;
		if(item->m_itemType.edibleForDrinkersOf == &m_actor.m_species.fluidType)
			return 2;
	}
	if(m_actor.m_species.eatsFruit && block.m_hasPlant.exists())
	{
		const Plant& plant = block.m_hasPlant.get();
		if(plant.getFruitMass() != 0)
			return 2;
	}
	if(m_actor.m_species.eatsMeat)
		for(Actor* actor : block.m_hasActors.getAll())
			if(&m_actor != actor && !actor->isAlive() && actor->m_species.fluidType == m_actor.m_species.fluidType)
				return 1;
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
Block* MustEat::getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability()
{
	constexpr uint32_t maxRankedEatDesire = 3;
	std::array<Block*, maxRankedEatDesire> candidates = {};
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		uint32_t eatDesire = getDesireToEatSomethingAt(block);
		if(eatDesire == UINT32_MAX)
			return true;
		if(eatDesire < getMinimumAcceptableDesire())
			return false;
		if(eatDesire != 0 && candidates[eatDesire - 1u] == nullptr)
			candidates[eatDesire - 1u] = const_cast<Block*>(&block);
		return false;
	};
	Block* output = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
	if(output != nullptr)
		return output;
	for(size_t i = maxRankedEatDesire; i != 0; --i)
		if(candidates[i - 1] != nullptr)
			return candidates[i - 1];
	return nullptr;
}
