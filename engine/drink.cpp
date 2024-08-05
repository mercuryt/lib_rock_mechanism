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
MustDrink::MustDrink(Area& area, ActorIndex a) :
	m_thirstEvent(area.m_eventSchedule)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
}
MustDrink::MustDrink(Area& area, const Json& data, ActorIndex a, const AnimalSpecies& species) : 
	m_thirstEvent(area.m_eventSchedule), m_fluidType(&species.fluidType),
       	m_volumeDrinkRequested(data["volumeDrinkRequested"].get<CollisionVolume>())
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
	if(data.contains("thirstEventStart"))
	{
		Step start = data["thirstEventStart"].get<Step>();
		m_thirstEvent.schedule(area, species.stepsFluidDrinkFreqency, m_actor.getIndex(), start);
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
void MustDrink::drink(Area& area, CollisionVolume volume)
{
	assert(m_volumeDrinkRequested >= volume);
	assert(m_volumeDrinkRequested != 0);
	assert(volume != 0);
	assert(m_objective != nullptr);
	m_volumeDrinkRequested -= volume;
	m_thirstEvent.unschedule();
	Step stepsToNextThirstEvent;
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex();
	if(m_volumeDrinkRequested == 0)
	{
		actors.objective_complete(actor, *m_objective);
		stepsToNextThirstEvent = actors.getSpecies(actor).stepsTillDieWithoutFluid;
		actors.grow_maybeStart(actor);
	}
	else
	{
		actors.objective_subobjectiveComplete(actor);
		const AnimalSpecies& species = actors.getSpecies(actor);
		stepsToNextThirstEvent = Step::create(util::scaleByFraction(species.stepsTillDieWithoutFluid.get(), m_volumeDrinkRequested.get(), species.stepsTillDieWithoutFluid.get()));
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
	ActorIndex actor = m_actor.getIndex();
	if(m_volumeDrinkRequested == 0)
	{
		m_volumeDrinkRequested = drinkVolumeFor(area, actor);
		const AnimalSpecies& species = actors.getSpecies(actor);
		m_thirstEvent.schedule(area, species.stepsTillDieWithoutFluid, actor);
		std::unique_ptr<Objective> objective = std::make_unique<DrinkObjective>(area);
		m_objective = static_cast<DrinkObjective*>(objective.get());
		actors.objective_addNeed(actor, std::move(objective));
	}
	else
		actors.die(actor, CauseOfDeath::thirst);
}
void MustDrink::onDeath()
{
	m_thirstEvent.maybeUnschedule();
}
void MustDrink::scheduleDrinkEvent(Area& area)
{
	ActorIndex actor = m_actor.getIndex();
	const AnimalSpecies& species = area.getActors().getSpecies(actor);
	m_thirstEvent.schedule(area, species.stepsFluidDrinkFreqency, actor);
}
void MustDrink::setFluidType(const FluidType& fluidType) { m_fluidType = &fluidType; }
Percent MustDrink::getPercentDeadFromThirst() const
{
	if(!m_thirstEvent.exists())
		return Percent::create(0);
	return m_thirstEvent.percentComplete();
}
CollisionVolume MustDrink::drinkVolumeFor(Area& area, ActorIndex actor) { return CollisionVolume::create(std::max(1u, area.getActors().getMass(actor).get() / Config::unitsBodyMassPerUnitFluidConsumed)); }
// Drink Event.
DrinkEvent::DrinkEvent(Area& area, const Step delay, DrinkObjective& drob, ActorIndex actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_drinkObjective(drob) 
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
DrinkEvent::DrinkEvent(Area& area, const Step delay, DrinkObjective& drob, ActorIndex actor, ItemIndex item, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_drinkObjective(drob)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
	m_item.setTarget(area.getItems().getReferenceTarget(item));
}
void DrinkEvent::execute(Simulation&, Area* area)
{
	ActorIndex actor = m_actor.getIndex();
	Actors& actors = area->getActors();
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
		const FluidType& fluidType = actors.drink_getFluidType(actor);
		volume = std::min(volume, blocks.fluid_volumeOfTypeContains(drinkBlock, fluidType));
		blocks.fluid_remove(drinkBlock, volume, fluidType);
	}
	actors.drink_do(actor, volume);
}
void DrinkEvent::clearReferences(Simulation&, Area*) { m_drinkObjective.m_drinkEvent.clearPointer(); }
ThirstEvent::ThirstEvent(Area& area, const Step delay, ActorIndex a, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_actor(a) { }
void ThirstEvent::execute(Simulation&, Area* area) { area->getActors().drink_setNeedsFluid(m_actor); }
void ThirstEvent::clearReferences(Simulation&, Area* area) { area->getActors().m_mustDrink.at(m_actor)->m_thirstEvent.clearPointer(); }
