#include "drink.h"
#include "area.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include "objectives/drink.h"
#include "actors/actors.h"
#include "items/items.h"
#include "blocks/blocks.h"
#include "animalSpecies.h"
#include <algorithm>
// Must Drink.
MustDrink::MustDrink(Area& area, const ActorIndex& a) :
	m_thirstEvent(area.m_eventSchedule)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
MustDrink::MustDrink(Area& area, const Json& data, const ActorIndex& a, const AnimalSpeciesId& species) : 
	m_thirstEvent(area.m_eventSchedule), m_fluidType(AnimalSpecies::getFluidType(species)),
       	m_volumeDrinkRequested(data["volumeDrinkRequested"].get<CollisionVolume>())
{
	const ActorReferenceData& referenceData = area.getActors().m_referenceData;
	m_actor.setIndex(a, referenceData);
	if(data.contains("thirstEventStart"))
	{
		Step start = data["thirstEventStart"].get<Step>();
		m_thirstEvent.schedule(area, AnimalSpecies::getStepsFluidDrinkFrequency(species), m_actor.getIndex(referenceData), start);
	}
}
Json MustDrink::toJson() const
{
	Json data;
	data["volumeDrinkRequested"] = m_volumeDrinkRequested;
	if(m_thirstEvent.exists())
		data["thirstEventStart"] = m_thirstEvent.getStartStep();
	return data;
}
void MustDrink::drink(Area& area, const CollisionVolume& volume)
{
	assert(m_volumeDrinkRequested >= volume);
	assert(m_volumeDrinkRequested != 0);
	assert(volume != 0);
	m_volumeDrinkRequested -= volume;
	m_thirstEvent.unschedule();
	Step stepsToNextThirstEvent;
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	Step stepsTillDie = AnimalSpecies::getStepsTillDieWithoutFluid(actors.getSpecies(actor));
	if(m_volumeDrinkRequested == 0)
	{
		// Might drink without objective if forced to while objective is supressed.
		if(m_objective != nullptr)
			actors.objective_complete(actor, *m_objective);
		stepsToNextThirstEvent = stepsTillDie;
		actors.grow_maybeStart(actor);
	}
	else
	{
		if(m_objective != nullptr)
			actors.objective_subobjectiveComplete(actor);
		//TODO: This doesn't seem to make sense.
		stepsToNextThirstEvent = Step::create(util::scaleByFraction(stepsTillDie.get(), drinkVolumeFor(area, actor).get(), m_volumeDrinkRequested.get()));
	}
	m_thirstEvent.schedule(area, stepsToNextThirstEvent, actor);
}
void MustDrink::notThirsty(Area& area)
{
	if(m_volumeDrinkRequested != 0)
		drink(area, m_volumeDrinkRequested);
}
void MustDrink::setNeedsFluid(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	if(m_volumeDrinkRequested == 0)
	{
		m_volumeDrinkRequested = drinkVolumeFor(area, actor);
		Step stepsTillDie = AnimalSpecies::getStepsTillDieWithoutFluid(actors.getSpecies(actor));
		m_thirstEvent.maybeUnschedule();
		m_thirstEvent.schedule(area, stepsTillDie, actor);
		std::unique_ptr<Objective> objective = std::make_unique<DrinkObjective>(area);
		m_objective = static_cast<DrinkObjective*>(objective.get());
		actors.objective_addNeed(actor, std::move(objective));
		actors.grow_updateGrowingStatus(actor);
	}
	else
		actors.die(actor, CauseOfDeath::thirst);
}
void MustDrink::unschedule()
{
	m_thirstEvent.maybeUnschedule();
}
void MustDrink::scheduleDrinkEvent(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	AnimalSpeciesId species = area.getActors().getSpecies(actor);
	Step frequency = AnimalSpecies::getStepsFluidDrinkFrequency(species);
	m_thirstEvent.schedule(area, frequency, actor);
}
void MustDrink::setFluidType(const FluidTypeId& fluidType) { m_fluidType = fluidType; }
Percent MustDrink::getPercentDead() const
{
	assert(needsFluid());
	return m_thirstEvent.percentComplete();
}
Step MustDrink::getStepsTillDead() const
{
	assert(needsFluid());
	return m_thirstEvent.remainingSteps();
}
CollisionVolume MustDrink::drinkVolumeFor(Area& area, const ActorIndex& actor) { return CollisionVolume::create(std::max(1u, area.getActors().getMass(actor).get() / Config::unitsBodyMassPerUnitFluidConsumed)); }
// Drink Event.
DrinkEvent::DrinkEvent(Area& area, const Step& delay, DrinkObjective& drob, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_drinkObjective(drob) 
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
DrinkEvent::DrinkEvent(Area& area, const Step& delay, DrinkObjective& drob, const ActorIndex& actor, const ItemIndex& item, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_drinkObjective(drob)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
	m_item.setIndex(item, area.getItems().m_referenceData);
}
void DrinkEvent::execute(Simulation&, Area* area)
{
	Actors& actors = area->getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	CollisionVolume volume = actors.drink_getVolumeOfFluidRequested(actor);
	BlockIndex drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(*area, actors.getLocation(actor), actors.getFacing(actor), actor);
	if(drinkBlock.empty())
	{
		// There isn't anything to drink here anymore, try again.
		m_drinkObjective.makePathRequest(*area, actor);
		return;
	}
	ItemIndex item = m_drinkObjective.getItemToDrinkFromAt(*area, drinkBlock, actor);
	Items& items = area->getItems();
	if(item.exists())
	{
		assert(items.cargo_getFluidType(item) == actors.drink_getFluidType(actor));
		volume = std::min(volume, items.cargo_getFluidVolume(item));
		items.cargo_removeFluid(item, volume);
	}
	else
	{
		Blocks& blocks = area->getBlocks();
		FluidTypeId fluidType = actors.drink_getFluidType(actor);
		volume = std::min(volume, blocks.fluid_volumeOfTypeContains(drinkBlock, fluidType));
		blocks.fluid_remove(drinkBlock, volume, fluidType);
	}
	actors.drink_do(actor, volume);
}
void DrinkEvent::clearReferences(Simulation&, Area*) { m_drinkObjective.m_drinkEvent.clearPointer(); }
ThirstEvent::ThirstEvent(Area& area, const Step& delay, const ActorIndex& a, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(a) { }
void ThirstEvent::execute(Simulation&, Area* area) { area->getActors().drink_setNeedsFluid(m_actor); }
void ThirstEvent::clearReferences(Simulation&, Area* area) { area->getActors().m_mustDrink[m_actor]->m_thirstEvent.clearPointer(); }
