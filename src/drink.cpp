#include "drink.h"
#include "actor.h"
#include "util.h"
#include "block.h"
#include <algorithm>
// Must Drink.
MustDrink::MustDrink(Actor& a) : m_actor(a), m_volumeDrinkRequested(0), m_fluidType(&m_actor.m_species.fluidType), m_objective(nullptr), m_thirstEvent(a.getEventSchedule())
{ 
	m_thirstEvent.schedule(m_actor.m_species.stepsFluidDrinkFreqency, m_actor);
}
void MustDrink::drink(uint32_t volume)
{
	assert(m_volumeDrinkRequested >= volume);
	assert(m_volumeDrinkRequested != 0);
	assert(volume != 0);
	assert(m_objective != nullptr);
	m_volumeDrinkRequested -= volume;
	m_thirstEvent.unschedule();
	Step stepsToNextThirstEvent;
	if(m_volumeDrinkRequested == 0)
	{
		m_actor.m_hasObjectives.objectiveComplete(*m_objective);
		stepsToNextThirstEvent = m_actor.m_species.stepsTillDieWithoutFluid;
		m_actor.m_canGrow.maybeStart();
	}
	else
		stepsToNextThirstEvent = util::scaleByFraction(m_actor.m_species.stepsTillDieWithoutFluid, m_volumeDrinkRequested, m_actor.m_species.stepsTillDieWithoutFluid);
	m_thirstEvent.schedule(stepsToNextThirstEvent, m_actor);
}
void MustDrink::setNeedsFluid()
{
	if(m_volumeDrinkRequested == 0)
	{
		m_volumeDrinkRequested = drinkVolumeFor(m_actor);
		m_thirstEvent.schedule(m_actor.m_species.stepsTillDieWithoutFluid, m_actor);
		std::unique_ptr<Objective> objective = std::make_unique<DrinkObjective>(m_actor);
		m_objective = static_cast<DrinkObjective*>(objective.get());
		m_actor.m_hasObjectives.addNeed(std::move(objective));
	}
	else
		m_actor.die(CauseOfDeath::thirst);
}
void MustDrink::onDeath()
{
	m_thirstEvent.maybeUnschedule();
}
// Static method.
uint32_t MustDrink::drinkVolumeFor(Actor& actor) { return std::max(1u, actor.getMass() / Config::unitsBodyMassPerUnitFluidConsumed); }
// Drink Event.
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob) : ScheduledEventWithPercent(drob.m_actor.getSimulation(), delay), m_drinkObjective(drob), m_item(nullptr) {}
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob, Item& i) : ScheduledEventWithPercent(drob.m_actor.getSimulation(), delay), m_drinkObjective(drob), m_item(&i) {}
void DrinkEvent::execute()
{
	auto& actor = m_drinkObjective.m_actor;
	uint32_t volume = actor.m_mustDrink.m_volumeDrinkRequested;
	Block* drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(*actor.m_location, actor.m_facing);
	if(drinkBlock == nullptr)
	{
		// There isn't anything to drink here anymore, try again.
		m_drinkObjective.execute();
		return;
	}
	Item* item = m_drinkObjective.getItemToDrinkFromAt(*drinkBlock);
	if(item != nullptr)
	{
		assert(item->m_hasCargo.getFluidType() == actor.m_mustDrink.getFluidType());
		volume = std::min(volume, item->m_hasCargo.getFluidVolume());
		item->m_hasCargo.removeFluidVolume(volume);
	}
	else
	{
		volume = std::min(volume, drinkBlock->volumeOfFluidTypeContains(actor.m_mustDrink.getFluidType()));
		drinkBlock->removeFluid(volume, actor.m_mustDrink.getFluidType());
	}
	assert(volume !=  0);
	actor.m_mustDrink.drink(volume);
}
void DrinkEvent::clearReferences() { m_drinkObjective.m_drinkEvent.clearPointer(); }
ThirstEvent::ThirstEvent(const Step delay, Actor& a) : ScheduledEventWithPercent(a.getSimulation(), delay), m_actor(a) { }
void ThirstEvent::execute() { m_actor.m_mustDrink.setNeedsFluid(); }
void ThirstEvent::clearReferences() { m_actor.m_mustDrink.m_thirstEvent.clearPointer(); }
// Drink Objective.
DrinkObjective::DrinkObjective(Actor& a) : Objective(Config::drinkPriority), m_actor(a), m_drinkEvent(a.getEventSchedule()), m_noDrinkFound(false) { }
void DrinkObjective::execute()
{
	if(m_noDrinkFound)
	{
		// We have determined that there is no drink here and have attempted to path to the edge of the area so we can leave.
		if(m_actor.predicateForAnyOccupiedBlock([](const Block& block){ return block.m_isEdge; }))
			// We are at the edge and can leave.
			m_actor.leaveArea();
		else
			// No drink and no escape.
			m_actor.m_hasObjectives.cannotFulfillNeed(*this);
		return;
	}
	std::function<bool(const Block&)> predicate = [&](const Block& block){ return containsSomethingDrinkable(block); };
	std::function<void(Block&)> callback = [&]([[maybe_unused]] Block& block) { m_drinkEvent.schedule(Config::stepsToDrink, *this); };
	m_actor.m_canMove.goToPredicateBlockAndThen(predicate, callback);
}
void DrinkObjective::cancel() 
{ 
	m_drinkEvent.maybeUnschedule();
	m_actor.m_mustDrink.m_objective = nullptr; 
}
void DrinkObjective::delay() 
{ 
	m_drinkEvent.maybeUnschedule();
}
Block* DrinkObjective::getAdjacentBlockToDrinkAt(const Block& location, Facing facing) const
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return containsSomethingDrinkable(block); };
	return m_actor.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
}
bool DrinkObjective::canDrinkItemAt(const Block& block) const
{
	return getItemToDrinkFromAt(const_cast<Block&>(block)) != nullptr;
}
Item* DrinkObjective::getItemToDrinkFromAt(Block& block) const
{
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_hasCargo.getFluidType() == m_actor.m_mustDrink.getFluidType())
			return item;
	return nullptr;
}
bool DrinkObjective::containsSomethingDrinkable(const Block& block) const
{
	return block.m_fluids.contains(&m_actor.m_mustDrink.getFluidType()) || canDrinkItemAt(block);
}
