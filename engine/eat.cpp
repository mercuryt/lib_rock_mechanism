#include "eat.h"
#include "actors/actors.h"
#include "definitions/animalSpecies.h"
#include "config.h"
#include "definitions/itemType.h"
#include "objective.h"
#include "area/area.h"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"
#include "util.h"
#include "simulation/simulation.h"
#include "objectives/kill.h"
#include "objectives/eat.h"
#include "plants.h"
#include "items/items.h"
#include "space/space.h"
HungerEvent::HungerEvent(Area& area, const Step& delay, const ActorIndex& a, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(a) { }
void HungerEvent::execute(Simulation&, Area* area)
{
	area->getActors().m_mustEat[m_actor].get()->setNeedsFood(*area);
}
void HungerEvent::clearReferences(Simulation&, Area* area)
{
	area->getActors().m_mustEat[m_actor].get()->m_hungerEvent.clearPointer();
}
MustEat::MustEat(Area& area, const ActorIndex& a) :
	m_hungerEvent(area.m_eventSchedule)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
void MustEat::scheduleHungerEvent(Area& area)
{
	const ActorReferenceData& referenceData = area.getActors().m_referenceData;
	Step eatFrequency = AnimalSpecies::getStepsEatFrequency(area.getActors().getSpecies(m_actor.getIndex(referenceData)));
	m_hungerEvent.schedule(area, eatFrequency, m_actor.getIndex(referenceData));
}
MustEat::MustEat(Area& area, const Json& data, const ActorIndex& actor, AnimalSpeciesId species) :
	m_hungerEvent(area.m_eventSchedule), m_massFoodRequested(data["massFoodRequested"].get<Mass>())
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
	if(data.contains("eatingLocation"))
		m_eatingLocation = data["eatingLocation"].get<Point3D>();
	if(data.contains("hungerEventStart"))
	{
		Step start = data["hungerEventStart"].get<Step>();
		m_hungerEvent.schedule(area, AnimalSpecies::getStepsEatFrequency(species), actor, start);
	}
}
Json MustEat::toJson() const
{
	Json data;
	data["massFoodRequested"] = m_massFoodRequested;
	if(m_eatingLocation.exists())
		data["eatingLocation"] = m_eatingLocation;
	data["hungerEventStart"] = m_hungerEvent.getStartStep();
	return data;
}
bool MustEat::canEatActor(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	if(actors.isAlive(actor))
		return false;
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	AnimalSpeciesId species = actors.getSpecies(m_actor.getIndex(referenceData));
	assert(AnimalSpecies::getEatsMeat(species));
	if(AnimalSpecies::getFluidType(species) != AnimalSpecies::getFluidType(actors.getSpecies(actor)))
		return false;
	return true;
}
bool MustEat::canEatPlant(Area& area, const PlantIndex& plant) const
{
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	AnimalSpeciesId species = area.getActors().getSpecies(m_actor.getIndex(referenceData));
	Plants& plants = area.getPlants();
	if(AnimalSpecies::getEatsLeaves(species) && plants.getPercentFoliage(plant) != 0)
		return true;
	if(AnimalSpecies::getEatsFruit(species) && plants.getQuantityToHarvest(plant) != 0)
		return true;
	return false;
}
bool MustEat::canEatItem(Area& area, const ItemIndex& item) const
{
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	return ItemType::getEdibleForDrinkersOf(area.getItems().getItemType(item)) == area.getActors().drink_getFluidType(m_actor.getIndex(referenceData));
}
void MustEat::eat(Area& area, Mass mass)
{
	assert(mass <= m_massFoodRequested);
	assert(mass != 0);
	assert(m_eatObjective!= nullptr);
	Actors& actors = area.getActors();
	m_massFoodRequested -= mass;
	m_hungerEvent.unschedule();
	Step stepsToNextHungerEvent;
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	ActorIndex actor = m_actor.getIndex(referenceData);
	AnimalSpeciesId species = actors.getSpecies(actor);
	if(m_massFoodRequested == 0)
	{
		actors.grow_maybeStart(actor);
		stepsToNextHungerEvent = AnimalSpecies::getStepsEatFrequency(species);
		actors.objective_complete(actor, *m_eatObjective);
		m_eatObjective = nullptr;
		m_hungerEvent.schedule(area, stepsToNextHungerEvent, actor);
	}
	else
	{
		Step stepsTillDie = AnimalSpecies::getStepsTillDieWithoutFood(species);
		stepsToNextHungerEvent = Step::create(util::scaleByInverseFraction(stepsTillDie.get(), m_massFoodRequested.get(), massFoodForBodyMass(area).get()));
		m_hungerEvent.schedule(area, stepsToNextHungerEvent, actor);
		actors.objective_subobjectiveComplete(actor);
	}
}
void MustEat::notHungry(Area& area)
{
	if(m_massFoodRequested != 0)
		eat(area, m_massFoodRequested);
}
void MustEat::setNeedsFood(Area& area)
{
	Actors& actors = area.getActors();
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	ActorIndex actor = m_actor.getIndex(referenceData);
	AnimalSpeciesId species = actors.getSpecies(actor);
	if(needsFood())
		actors.die(actor, CauseOfDeath::hunger);
	else
	{
		m_hungerEvent.maybeUnschedule();
		m_hungerEvent.schedule(area, AnimalSpecies::getStepsTillDieWithoutFood(species), actor);
		actors.grow_stop(actor);
		m_massFoodRequested = massFoodForBodyMass(area);
		std::unique_ptr<Objective> objective = std::make_unique<EatObjective>(area);
		m_eatObjective = static_cast<EatObjective*>(objective.get());
		actors.objective_addNeed(actor, std::move(objective));
		actors.grow_updateGrowingStatus(actor);
	}
}
void MustEat::unschedule()
{
	m_hungerEvent.maybeUnschedule();
}
void MustEat::setObjective(EatObjective& objective) { assert(m_eatObjective == nullptr); m_eatObjective = &objective; }
bool MustEat::needsFood() const { return m_massFoodRequested != 0; }
Mass MustEat::massFoodForBodyMass(Area& area) const
{
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	return Mass::create(std::max(1u, (area.getActors().getMass(m_actor.getIndex(referenceData)) / Config::unitsBodyMassPerUnitFoodConsumed).get()));
}
Mass MustEat::getMassFoodRequested() const { return m_massFoodRequested; }
Percent MustEat::getPercentStarved() const
{
	if(!m_hungerEvent.exists())
		return Percent::create(0);
	return m_hungerEvent.percentComplete();
}
uint32_t MustEat::getDesireToEatSomethingAt(Area& area, const Point3D& point) const
{
	Space& space =  area.getSpace();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	AnimalSpeciesId species = actors.getSpecies(m_actor.getIndex(referenceData));
	for(ItemIndex item : space.item_getAll(point))
	{
		if(items.isPreparedMeal(item))
			return maxRankedEatDesire;
		if(ItemType::getEdibleForDrinkersOf(items.getItemType(item)) == AnimalSpecies::getFluidType(species))
			return 2;
	}
	if(AnimalSpecies::getEatsFruit(species) && space.plant_exists(point))
	{
		const PlantIndex plant = space.plant_get(point);
		if(area.getPlants().getFruitMass(plant) != 0)
			return 2;
	}
	if(AnimalSpecies::getEatsMeat(species))
	{
		const ActorReferenceData &referenceData = area.getActors().m_referenceData;
		for(ActorIndex actor : space.actor_getAll(point))
			if(m_actor.getIndex(referenceData) != actor && !actors.isAlive(actor) && AnimalSpecies::getFluidType(actors.getSpecies(actor)) == AnimalSpecies::getFluidType(species))
				return 1;
	}
	if(AnimalSpecies::getEatsLeaves(species) && space.plant_exists(point))
		if(area.getPlants().getPercentFoliage(space.plant_get(point)) != 0)
			return 3;
	return 0;
}
uint32_t MustEat::getMinimumAcceptableDesire(Area& area) const
{
	assert(m_hungerEvent.exists());
	// Sentients demand max rank ( prepared meals) to start, but become less picky as they get more hungry.
	// Non sentients still prefer prepared meals if they can get to them, but are willing to scavange by default.
	Percent hunger = getPercentStarved();
	const ActorReferenceData &referenceData = area.getActors().m_referenceData;
	for(uint i = 0; i < Config::minimumHungerLevelThresholds.size(); ++i )
	{
		if(hunger < Config::minimumHungerLevelThresholds[i])
		{
			// max ranked desire is reserved for sentients eating prepared meals. Nonsentients will forage at any hunger level.
			if(i == maxRankedEatDesire && !area.getActors().isSentient(m_actor.getIndex(referenceData)) )
				return maxRankedEatDesire - 1;
			return maxRankedEatDesire - i;
		}
	}
	return 0;
}
Point3D MustEat::getAdjacentPointWithHighestDesireFoodOfAcceptableDesireability(Area& area)
{
	std::array<Point3D, maxRankedEatDesire> candidates;
	candidates.fill(Point3D::null());
	ActorIndex actor = m_actor.getIndex(area.getActors().m_referenceData);
	//TODO: abstract predicate generation from PathRequest constructor and use here as well.
	std::function<bool(const Point3D&)> predicate;
	if(area.getActors().isSentient(actor))
		predicate = [&](const Point3D& point)
		{
			uint32_t eatDesire = getDesireToEatSomethingAt(area, point);
			if(eatDesire < getMinimumAcceptableDesire(area))
				return false;
			if(eatDesire == maxRankedEatDesire)
				return true;
			if(eatDesire != 0 && candidates[eatDesire - 1u].empty())
				candidates[eatDesire - 1u] = point;
			return false;
		};
	else
		// Nonsentients aren't picky.
		predicate = [&](const Point3D& point)
		{
			return getDesireToEatSomethingAt(area, point) != 0;
		};
	Point3D output = area.getActors().getPointWhichIsAdjacentWithPredicate(actor, predicate);
	if(output.exists())
		return output;
	for(size_t i = maxRankedEatDesire; i != 0; --i)
		if(candidates[i - 1].exists())
			return candidates[i - 1];
	return Point3D::null();
}
