#include "drink.h"
#include "path.h"
#include "util.h"
// Must Drink.
MustDrink::MustDrink(Actor& a) : m_actor(a), m_volumeDrinkRequested(0), m_objective(nullptr)
{ 
	m_thirstEvent.schedule(m_actor.m_species.stepsFluidDrinkFreqency, m_actor);
}
void MustDrink::drink(uint32_t volume)
{
	assert(m_volumeDrinkRequested >= volume);
	m_volumeDrinkRequested -= volume;
	m_thirstEvent.unschedule();
	uint32_t stepsToNextThirsEvent;
	if(m_volumeDrinkRequested == 0)
	{
		m_actor.m_hasObjectives.objectiveComplete(*m_objective);
		stepsToNextThirsEvent = m_actor.m_species.stepsFluidDrinkFreqency;
	}
	else
		stepsToNextThirsEvent = util::scaleByFraction(m_actor.m_species.stepsTillDieWithoutFluid, m_volumeDrinkRequested, m_actor.m_species.stepsTillDieWithoutFluid);
	m_thirstEvent.schedule(stepsToNextThirsEvent, m_actor);
}
void MustDrink::setNeedsFluid()
{
	if(m_volumeDrinkRequested == 0)
		m_volumeDrinkRequested = m_actor.getMass() * Config::volumeFluidForBodyMass;
	else
		m_actor.die(CauseOfDeath::thirst);
}
// Drink Event.
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob) : ScheduledEventWithPercent(delay), m_drinkObjective(drob), m_item(nullptr) {}
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob, Item& i) : ScheduledEventWithPercent(delay), m_drinkObjective(drob), m_item(&i) {}
void DrinkEvent::execute()
{
	//TODO: What if not enough volume is avalible?
	auto& actor = m_drinkObjective.m_actor;
	uint32_t volume = actor.m_mustDrink.m_volumeDrinkRequested;
	if(m_item != nullptr)
	{
		if(m_item->m_hasCargo.getFluidType() != actor.m_mustDrink.getFluidType())
		{
			m_drinkObjective.m_threadedTask.create(m_drinkObjective);
			return;
		}
		m_item->m_hasCargo.remove(actor.m_mustDrink.getFluidType(), volume);
		m_item->m_reservable.clearReservationFor(actor.m_canReserve);
	}
	else
	{
		Block* drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(*actor.m_location);
		if(drinkBlock == nullptr)
		{
			m_drinkObjective.m_threadedTask.create(m_drinkObjective);
			return;
		}
		drinkBlock->removeFluid(volume, actor.m_mustDrink.getFluidType());
	}
	actor.m_mustDrink.drink(volume);
	actor.m_hasObjectives.objectiveComplete(m_drinkObjective);
}
void DrinkEvent::clearReferences() { m_drinkObjective.m_drinkEvent.clearPointer(); }
void ThirstEvent::execute() { m_actor.m_mustDrink.setNeedsFluid(); }
void ThirstEvent::clearReferences() { m_actor.m_mustDrink.m_thirstEvent.clearPointer(); }
// Drink Threaded Task.
void DrinkThreadedTask::readStep()
{
	auto destinationCondition = [&](Block& block)
	{
		return m_drinkObjective.canDrinkAt(block) || m_drinkObjective.canDrinkItemAt(block);
	};
	m_result = path::getForActorToPredicate(m_drinkObjective.m_actor, destinationCondition);
}
void DrinkThreadedTask::writeStep()
{
	if(m_result.empty())
		m_drinkObjective.m_actor.m_hasObjectives.cannotFulfillObjective(m_drinkObjective);
	else
		m_drinkObjective.m_actor.m_canMove.setPath(m_result);
}
void DrinkThreadedTask::clearReferences() { m_drinkObjective.m_threadedTask.clearPointer(); }
// Drink Objective.
DrinkObjective::DrinkObjective(Actor& a) : Objective(Config::drinkPriority), m_actor(a) { }
void DrinkObjective::execute()
{
	Item* item = getItemToDrinkFromAt(*m_actor.m_location);
	if(item != nullptr)
	{
		item->m_reservable.reserveFor(m_actor.m_canReserve, 1);
		m_drinkEvent.schedule(Config::stepsToDrink, *this, *item);
	}
	else
	{
		if(!canDrinkAt(*m_actor.m_location))
			m_threadedTask.create(*this);
		else
			m_drinkEvent.schedule(Config::stepsToDrink, *this);
	}
}
bool DrinkObjective::canDrinkAt(const Block& block) const
{
	return getAdjacentBlockToDrinkAt(block) != nullptr;
}
Block* DrinkObjective::getAdjacentBlockToDrinkAt(const Block& block) const
{
	for(Block* adjacent : block.m_adjacentsVector)
		if(adjacent->m_fluids.contains(&m_actor.m_mustDrink.getFluidType()))
			return adjacent;
	return nullptr;
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
