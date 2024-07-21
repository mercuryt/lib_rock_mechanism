#include "eat.h"
#include "actors/actors.h"
#include "animalSpecies.h"
#include "config.h"
#include "itemType.h"
#include "objective.h"
#include "area.h"
#include "terrainFacade.h"
#include "types.h"
#include "util.h"
#include "simulation.h"
#include "objectives/kill.h"
#include "objectives/eat.h"
#include "plants.h"
#include "items/items.h"
#include "blocks/blocks.h"
HungerEvent::HungerEvent(Area& area, const Step delay, ActorIndex a, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(a) { }
void HungerEvent::execute(Simulation&, Area* area)
{
	area->getActors().m_mustEat.at(m_actor).get()->setNeedsFood(*area);
}
void HungerEvent::clearReferences(Simulation&, Area* area) 
{ 
	area->getActors().m_mustEat.at(m_actor).get()->m_hungerEvent.clearPointer(); 
}
MustEat::MustEat(Area& area, ActorIndex a) : 
	m_hungerEvent(area.m_eventSchedule)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
}
void MustEat::scheduleHungerEvent(Area& area)
{
	m_hungerEvent.schedule(area, area.getActors().getSpecies(m_actor.getIndex()).stepsEatFrequency, m_actor.getIndex());
}
MustEat::MustEat(Area& area, const Json& data, ActorIndex a, const AnimalSpecies& species) : 
	m_hungerEvent(area.m_eventSchedule), m_massFoodRequested(data["massFoodRequested"].get<Mass>())
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
	if(data.contains("eatingLocation"))
		m_eatingLocation = data["eatingLocation"].get<BlockIndex>();
	if(data.contains("hungerEventStart"))
	{
		Step start = data["hungerEventStart"].get<Step>();
		m_hungerEvent.schedule(area, species.stepsEatFrequency, m_actor.getIndex(), start);
	}
}
Json MustEat::toJson() const
{
	Json data;
	data["massFoodRequested"] = m_massFoodRequested;
	if(m_eatingLocation != BLOCK_INDEX_MAX)
		data["eatingLocation"] = m_eatingLocation;
	data["hungerEventStart"] = m_hungerEvent.getStartStep();
	return data;
}
bool MustEat::canEatActor(Area& area, const ActorIndex actor) const
{
	Actors& actors = area.getActors();
	if(actors.isAlive(actor))
		return false;
	const AnimalSpecies& species = actors.getSpecies(m_actor.getIndex());
	if(!species.eatsMeat)
		return false;
	if(species.fluidType != actors.getSpecies(actor).fluidType)
		return false;
	return true;
}
bool MustEat::canEatPlant(Area& area, const PlantIndex plant) const
{
	const AnimalSpecies& species = area.getActors().getSpecies(m_actor.getIndex());
	Plants& plants = area.getPlants();
	if(species.eatsLeaves && plants.getPercentFoliage(plant) != 0)
		return true;
	if(species.eatsFruit && plants.getQuantityToHarvest(plant) != 0)
		return true;
	return false;
}
bool MustEat::canEatItem(Area& area, const ItemIndex item) const
{
	return area.getItems().getItemType(item).edibleForDrinkersOf == &area.getActors().drink_getFluidType(m_actor.getIndex());
}
void MustEat::eat(Area& area, Mass mass)
{
	assert(mass <= m_massFoodRequested);
	assert(mass != 0);
	Actors& actors = area.getActors();
	m_massFoodRequested -= mass;
	m_hungerEvent.unschedule();
	Step stepsToNextHungerEvent;
	ActorIndex actor = m_actor.getIndex();
	const AnimalSpecies& species = actors.getSpecies(actor);
	if(m_massFoodRequested == 0)
	{
		actors.grow_maybeStart(actor);
		stepsToNextHungerEvent = species.stepsEatFrequency;
		actors.objective_complete(actor, *m_eatObjective);
		m_eatObjective = nullptr;
		m_hungerEvent.schedule(area, stepsToNextHungerEvent, actor);
	}
	else
	{
		stepsToNextHungerEvent = util::scaleByInverseFraction(species.stepsTillDieWithoutFood, m_massFoodRequested, massFoodForBodyMass(area));
		m_hungerEvent.schedule(area, stepsToNextHungerEvent, actor);
		actors.objective_subobjectiveComplete(actor);
	}
}
void MustEat::notHungry(Area& area)
{
	if(m_massFoodRequested)
		eat(area, m_massFoodRequested);
}
void MustEat::setNeedsFood(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex();
	const AnimalSpecies& species = actors.getSpecies(actor);
	if(needsFood())
		actors.die(actor, CauseOfDeath::hunger);
	else
	{
		m_hungerEvent.schedule(area, species.stepsTillDieWithoutFood, actor);
		actors.grow_stop(actor);
		m_massFoodRequested = massFoodForBodyMass(area);
		std::unique_ptr<Objective> objective = std::make_unique<EatObjective>(area);
		m_eatObjective = static_cast<EatObjective*>(objective.get());
		actors.objective_addNeed(actor, std::move(objective));
	}
}
void MustEat::onDeath()
{
	m_hungerEvent.maybeUnschedule();
}
bool MustEat::needsFood() const { return m_massFoodRequested != 0; }
uint32_t MustEat::massFoodForBodyMass(Area& area) const
{
	return std::max(1u, area.getActors().getMass(m_actor.getIndex()) / Config::unitsBodyMassPerUnitFoodConsumed);
}
const uint32_t& MustEat::getMassFoodRequested() const { return m_massFoodRequested; }
Percent MustEat::getPercentStarved() const
{
	if(!m_hungerEvent.exists())
		return 0;
	return m_hungerEvent.percentComplete();
}
uint32_t MustEat::getDesireToEatSomethingAt(Area& area, BlockIndex block) const
{
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	const AnimalSpecies& species = actors.getSpecies(m_actor.getIndex());
	for(ItemIndex item : blocks.item_getAll(block))
	{
		if(items.isPreparedMeal(item))
			return UINT32_MAX;
		if(items.getItemType(item).edibleForDrinkersOf == &species.fluidType)
			return 2;
	}
	if(species.eatsFruit && blocks.plant_exists(block))
	{
		const PlantIndex plant = blocks.plant_get(block);
		if(area.getPlants().getFruitMass(plant) != 0)
			return 2;
	}
	if(species.eatsMeat)
		for(ActorIndex actor : blocks.actor_getAll(block))
			if(m_actor.getIndex() != actor && !actors.isAlive(actor) && actors.getSpecies(actor).fluidType == species.fluidType)
				return 1;
	if(species.eatsLeaves && blocks.plant_exists(block))
		if(area.getPlants().getPercentFoliage(blocks.plant_get(block)) != 0)
			return 3;
	return 0;
}
uint32_t MustEat::getMinimumAcceptableDesire() const
{
	assert(m_hungerEvent.exists());
	return m_hungerEvent.percentComplete() * (3 - Config::percentHungerAcceptableDesireModifier);
}
BlockIndex MustEat::getAdjacentBlockWithHighestDesireFoodOfAcceptableDesireability(Area& area)
{
	constexpr uint32_t maxRankedEatDesire = 3;
	std::array<BlockIndex, maxRankedEatDesire> candidates;
	candidates.fill(BLOCK_INDEX_MAX);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
	{
		uint32_t eatDesire = getDesireToEatSomethingAt(area, block);
		if(eatDesire == UINT32_MAX)
			return true;
		if(eatDesire < getMinimumAcceptableDesire())
			return false;
		if(eatDesire != 0 && candidates[eatDesire - 1u] == BLOCK_INDEX_MAX)
			candidates[eatDesire - 1u] = block;
		return false;
	};
	BlockIndex output = area.getActors().getBlockWhichIsAdjacentWithPredicate(m_actor.getIndex(), predicate);
	if(output != BLOCK_INDEX_MAX)
		return output;
	for(size_t i = maxRankedEatDesire; i != 0; --i)
		if(candidates[i - 1] != BLOCK_INDEX_MAX)
			return candidates[i - 1];
	return BLOCK_INDEX_MAX;
}
