#include "drink.h"
#include "actor.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "simulation.h"
#include "util.h"
#include "block.h"
#include <algorithm>
// Must Drink.
MustDrink::MustDrink(Actor& a) : m_actor(a), m_volumeDrinkRequested(0), m_fluidType(&m_actor.m_species.fluidType), m_objective(nullptr), m_thirstEvent(a.getEventSchedule())
{ 
	m_thirstEvent.schedule(m_actor.m_species.stepsFluidDrinkFreqency, m_actor);
}
MustDrink::MustDrink(const Json& data, Actor& a) : m_actor(a), m_volumeDrinkRequested(data["volumeDrinkRequested"].get<Volume>()), m_fluidType(&m_actor.m_species.fluidType), m_objective(nullptr), m_thirstEvent(a.getEventSchedule())
{
	if(data.contains("thirstEventStart"))
		m_thirstEvent.schedule(m_actor.m_species.stepsFluidDrinkFreqency, m_actor, data["thirstEventStart"].get<Step>());
}
Json MustDrink::toJson() const
{
	Json data;
	data["volumeDrinkRequested"] = m_volumeDrinkRequested;
	if(m_thirstEvent.exists())
		data["thirstEventStart"] = m_thirstEvent.getStartStep();
	return data;
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
	{
		m_actor.m_hasObjectives.subobjectiveComplete();
		stepsToNextThirstEvent = util::scaleByFraction(m_actor.m_species.stepsTillDieWithoutFluid, m_volumeDrinkRequested, m_actor.m_species.stepsTillDieWithoutFluid);
	}
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
uint32_t MustDrink::drinkVolumeFor(Actor& actor) { return std::max(1u, actor.getMass() / Config::unitsBodyMassPerUnitFluidConsumed); }
// Drink Event.
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob, const Step start) : ScheduledEvent(drob.m_actor.getSimulation(), delay, start), m_drinkObjective(drob), m_item(nullptr) {}
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob, Item& i, const Step start) : ScheduledEvent(drob.m_actor.getSimulation(), delay, start), m_drinkObjective(drob), m_item(&i) {}
void DrinkEvent::execute()
{
	auto& actor = m_drinkObjective.m_actor;
	uint32_t volume = actor.m_mustDrink.m_volumeDrinkRequested;
	Block* drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(*actor.m_location, actor.m_facing);
	if(drinkBlock == nullptr)
	{
		// There isn't anything to drink here anymore, try again.
		m_drinkObjective.m_threadedTask.create(m_drinkObjective);
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
	actor.m_mustDrink.drink(volume);
}
void DrinkEvent::clearReferences() { m_drinkObjective.m_drinkEvent.clearPointer(); }
ThirstEvent::ThirstEvent(const Step delay, Actor& a, const Step start) : ScheduledEvent(a.getSimulation(), delay, start), m_actor(a) { }
void ThirstEvent::execute() { m_actor.m_mustDrink.setNeedsFluid(); }
void ThirstEvent::clearReferences() { m_actor.m_mustDrink.m_thirstEvent.clearPointer(); }
// Drink Threaded Task.
DrinkThreadedTask::DrinkThreadedTask(DrinkObjective& drob) : ThreadedTask(drob.m_actor.getThreadedTaskEngine()), m_drinkObjective(drob), m_findsPath(drob.m_actor, drob.m_detour), m_noDrinkFound(false) {}
void DrinkThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block)
	{
		return m_drinkObjective.containsSomethingDrinkable(block);
	};
	if(m_drinkObjective.m_actor.getFaction())
		m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *m_drinkObjective.m_actor.getFaction());
	else
		m_findsPath.pathToAdjacentToPredicate(predicate);
	if(!m_findsPath.found() && !m_findsPath.m_useCurrentLocation)
	{
		// Nothing to drink here, try to leave.
		m_findsPath.pathToAreaEdge();
		m_noDrinkFound = true;
	}
}
void DrinkThreadedTask::writeStep()
{
	if(!m_findsPath.found())
	{
		if(m_findsPath.m_useCurrentLocation)
			m_drinkObjective.m_actor.m_hasObjectives.subobjectiveComplete();
		else
			m_drinkObjective.m_actor.m_hasObjectives.cannotFulfillNeed(m_drinkObjective);
	}
	else
	{
		if(!m_findsPath.areAllBlocksAtDestinationReservable(m_drinkObjective.m_actor.getFaction()))
		{
			m_drinkObjective.m_threadedTask.create(m_drinkObjective);
			return;
		}
		m_drinkObjective.m_actor.m_canMove.setPath(m_findsPath.getPath());
	}
	// Need task becomes exit area.
	if(m_noDrinkFound)
		m_drinkObjective.m_noDrinkFound = true;
}
void DrinkThreadedTask::clearReferences() { m_drinkObjective.m_threadedTask.clearPointer(); }
// Drink Objective.
DrinkObjective::DrinkObjective(Actor& a) : Objective(a, Config::drinkPriority), m_threadedTask(a.getThreadedTaskEngine()), m_drinkEvent(a.getEventSchedule()), m_noDrinkFound(false) { }
DrinkObjective::DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), m_drinkEvent(deserializationMemo.m_simulation.m_eventSchedule), m_noDrinkFound(data["noDrinkFound"].get<bool>())
{ 
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
	if(data.contains("eventStart"))
		m_drinkEvent.schedule(Config::stepsToDrink, *this, data["eventStart"].get<Step>());
}
Json DrinkObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noDrinkFound"] = m_noDrinkFound;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_drinkEvent.exists())
		data["eventStart"] = m_drinkEvent.getStartStep();
	return data;
}
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
	if(!canDrinkAt(*m_actor.m_location, m_actor.m_facing))
		m_threadedTask.create(*this);
	else
		m_drinkEvent.schedule(Config::stepsToDrink, *this);
}
void DrinkObjective::cancel() 
{ 
	m_threadedTask.maybeCancel();
	m_drinkEvent.maybeUnschedule();
	m_actor.m_mustDrink.m_objective = nullptr; 
}
void DrinkObjective::delay() 
{ 
	m_threadedTask.maybeCancel();
	m_drinkEvent.maybeUnschedule();
}
void DrinkObjective::reset()
{
	delay();
	m_noDrinkFound = false;
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
bool DrinkObjective::canDrinkAt(const Block& block, Facing facing) const
{
	return getAdjacentBlockToDrinkAt(block, facing) != nullptr;
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
