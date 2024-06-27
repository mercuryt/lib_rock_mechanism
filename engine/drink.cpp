#include "drink.h"
#include "area.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "simulation.h"
#include "terrainFacade.h"
#include "types.h"
#include "util.h"
#include "objectives/drink.h"
#include <algorithm>
// Must Drink.
MustDrink::MustDrink(Area& area, ActorIndex a) :
	m_thirstEvent(area.m_eventSchedule), m_actor(a) { }
/*
MustDrink::MustDrink(const Json& data, ActorIndex a, Simulation& s, const AnimalSpecies& species) : 
	m_thirstEvent(s.m_eventSchedule), m_actor(a), m_fluidType(&species.fluidType),
       	m_volumeDrinkRequested(data["volumeDrinkRequested"].get<Volume>())
{
	if(data.contains("thirstEventStart"))
	{
		Step start = data["thirstEventStart"].get<Step>();
		m_thirstEvent.schedule(species.stepsFluidDrinkFreqency, m_actor, start);
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
*/
void MustDrink::drink(Area& area, uint32_t volume)
{
	assert(m_volumeDrinkRequested >= volume);
	assert(m_volumeDrinkRequested != 0);
	assert(volume != 0);
	assert(m_objective != nullptr);
	m_volumeDrinkRequested -= volume;
	m_thirstEvent.unschedule();
	Step stepsToNextThirstEvent;
	Actors& actors = area.getActors();
	if(m_volumeDrinkRequested == 0)
	{
		actors.objective_complete(m_actor, *m_objective);
		stepsToNextThirstEvent = actors.getSpecies(m_actor).stepsTillDieWithoutFluid;
		actors.grow_maybeStart(m_actor);
	}
	else
	{
		actors.objective_subobjectiveComplete(m_actor);
		const AnimalSpecies& species = actors.getSpecies(m_actor);
		stepsToNextThirstEvent = util::scaleByFraction(species.stepsTillDieWithoutFluid, m_volumeDrinkRequested, species.stepsTillDieWithoutFluid);
	}
	m_thirstEvent.schedule(stepsToNextThirstEvent, m_actor);
}
void MustDrink::notThirsty(Area& area)
{
	if(m_volumeDrinkRequested)
		drink(area, m_volumeDrinkRequested);
}
void MustDrink::setNeedsFluid(Area& area)
{
	Actors& actors = area.getActors();
	if(m_volumeDrinkRequested == 0)
	{
		m_volumeDrinkRequested = drinkVolumeFor(area, m_actor);
		const AnimalSpecies& species = actors.getSpecies(m_actor);
		m_thirstEvent.schedule(species.stepsTillDieWithoutFluid, m_actor);
		std::unique_ptr<Objective> objective = std::make_unique<DrinkObjective>(area, m_actor);
		m_objective = static_cast<DrinkObjective*>(objective.get());
		actors.objective_addNeed(m_actor, std::move(objective));
	}
	else
		actors.die(m_actor, CauseOfDeath::thirst);
}
void MustDrink::onDeath()
{
	m_thirstEvent.maybeUnschedule();
}
void MustDrink::scheduleDrinkEvent(Area& area)
{
	const AnimalSpecies& species = area.getActors().getSpecies(m_actor);
	m_thirstEvent.schedule(species.stepsFluidDrinkFreqency, m_actor);
}
void MustDrink::setFluidType(const FluidType& fluidType) { m_fluidType = &fluidType; }
Percent MustDrink::getPercentDeadFromThirst() const
{
	if(!m_thirstEvent.exists())
		return 0;
	return m_thirstEvent.percentComplete();
}
uint32_t MustDrink::drinkVolumeFor(Area& area, ActorIndex actor) { return std::max(1u, area.getActors().getMass(actor) / Config::unitsBodyMassPerUnitFluidConsumed); }
// Drink Event.
DrinkEvent::DrinkEvent(Area& area, const Step delay, DrinkObjective& drob, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_drinkObjective(drob) {}
DrinkEvent::DrinkEvent(Area& area, const Step delay, DrinkObjective& drob, ItemIndex i, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_drinkObjective(drob), m_item(i) {}
void DrinkEvent::execute(Simulation&, Area* area)
{
	ActorIndex actor = m_drinkObjective.m_actor;
	Actors& actors = area->getActors();
	uint32_t volume = actors.drink_getVolumeOfFluidRequested(actor);
	BlockIndex drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(*area, actors.getLocation(actor), actors.getFacing(actor));
	if(drinkBlock == BLOCK_INDEX_MAX)
	{
		// There isn't anything to drink here anymore, try again.
		m_drinkObjective.makePathRequest(*area);
		return;
	}
	ItemIndex item = m_drinkObjective.getItemToDrinkFromAt(*area, drinkBlock);
	Items& items = area->getItems();
	if(item != ITEM_INDEX_MAX)
	{
		assert(items.cargo_getFluidType(item) == actors.drink_getFluidType(actor));
		volume = std::min(volume, items.cargo_getFluidVolume(item));
		items.cargo_removeFluid(actor, volume);
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
