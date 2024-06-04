#include "drink.h"
#include "item.h"
#include "area.h"
#include "actor.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include <algorithm>
// Must Drink.
MustDrink::MustDrink(Actor& a, Simulation& s) : m_thirstEvent(s.m_eventSchedule), m_actor(a) { }
MustDrink::MustDrink(const Json& data, Actor& a, Simulation& s, const AnimalSpecies& species) : 
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
void MustDrink::notThirsty()
{
	if(m_volumeDrinkRequested)
		drink(m_volumeDrinkRequested);
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
void MustDrink::scheduleDrinkEvent()
{
	m_thirstEvent.schedule(m_actor.m_species.stepsFluidDrinkFreqency, m_actor);
}
void MustDrink::setFluidType(const FluidType& fluidType) { m_fluidType = &fluidType; }
Percent MustDrink::getPercentDeadFromThirst() const
{
	if(!m_thirstEvent.exists())
		return 0;
	return m_thirstEvent.percentComplete();
}
uint32_t MustDrink::drinkVolumeFor(Actor& actor) { return std::max(1u, actor.getMass() / Config::unitsBodyMassPerUnitFluidConsumed); }
// Drink Event.
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob, const Step start) : ScheduledEvent(drob.m_actor.getSimulation(), delay, start), m_drinkObjective(drob), m_item(nullptr) {}
DrinkEvent::DrinkEvent(const Step delay, DrinkObjective& drob, Item& i, const Step start) : ScheduledEvent(drob.m_actor.getSimulation(), delay, start), m_drinkObjective(drob), m_item(&i) {}
void DrinkEvent::execute()
{
	auto& actor = m_drinkObjective.m_actor;
	uint32_t volume = actor.m_mustDrink.m_volumeDrinkRequested;
	BlockIndex drinkBlock = m_drinkObjective.getAdjacentBlockToDrinkAt(actor.m_location, actor.m_facing);
	if(drinkBlock == BLOCK_INDEX_MAX)
	{
		// There isn't anything to drink here anymore, try again.
		m_drinkObjective.m_threadedTask.create(m_drinkObjective);
		return;
	}
	Item* item = m_drinkObjective.getItemToDrinkFromAt(drinkBlock);
	if(item != nullptr)
	{
		assert(item->m_hasCargo.getFluidType() == actor.m_mustDrink.getFluidType());
		volume = std::min(volume, item->m_hasCargo.getFluidVolume());
		item->m_hasCargo.removeFluidVolume(volume);
	}
	else
	{
		auto& blocks = actor.m_area->getBlocks();
		volume = std::min(volume, blocks.fluid_volumeOfTypeContains(drinkBlock, actor.m_mustDrink.getFluidType()));
		blocks.fluid_remove(drinkBlock, volume, actor.m_mustDrink.getFluidType());
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
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
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
		if(m_actor.predicateForAnyOccupiedBlock([this](BlockIndex block){ return m_actor.m_area->getBlocks().isEdge(block); }))
			// We are at the edge and can leave.
			m_actor.leaveArea();
		else
			// No drink and no escape.
			m_actor.m_hasObjectives.cannotFulfillNeed(*this);
		return;
	}
	if(!canDrinkAt(m_actor.m_location, m_actor.m_facing))
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
bool DrinkObjective::canDrinkAt(BlockIndex block, Facing facing) const
{
	return getAdjacentBlockToDrinkAt(block, facing) != BLOCK_INDEX_MAX;
}
BlockIndex DrinkObjective::getAdjacentBlockToDrinkAt(BlockIndex location, Facing facing) const
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return containsSomethingDrinkable(block); };
	return m_actor.getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate);
}
bool DrinkObjective::canDrinkItemAt(BlockIndex block) const
{
	return getItemToDrinkFromAt(block) != nullptr;
}
Item* DrinkObjective::getItemToDrinkFromAt(BlockIndex block) const
{
	for(Item* item : m_actor.m_area->getBlocks().item_getAll(block))
		if(item->m_hasCargo.containsAnyFluid() && item->m_hasCargo.getFluidType() == m_actor.m_mustDrink.getFluidType())
			return item;
	return nullptr;
}
bool DrinkObjective::containsSomethingDrinkable(BlockIndex block) const
{
	return m_actor.m_area->getBlocks().fluid_contains(block, m_actor.m_mustDrink.getFluidType()) || canDrinkItemAt(block);
}
