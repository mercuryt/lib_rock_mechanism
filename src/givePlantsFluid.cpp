#include "givePlantsFluid.h"
#include "area.h"
#include "config.h"
GivePlantsFluidEvent::GivePlantsFluidEvent(Step delay, GivePlantsFluidObjective& gpfo) : ScheduledEventWithPercent(gpfo.m_actor.getSimulation(), delay), m_objective(gpfo) { }
void GivePlantsFluidEvent::execute()
{
	Plant& plant = m_objective.m_plantLocation->m_hasPlant.get();
	assert(m_objective.m_actor.m_canPickup.isCarryingFluidType(plant.m_plantSpecies.fluidType));
	uint32_t quantity = std::min(plant.getVolumeFluidRequested(), m_objective.m_actor.m_canPickup.getFluidVolume());
	plant.addFluid(quantity, m_objective.m_actor.m_canPickup.getFluidType());
	m_objective.m_actor.m_canPickup.removeFluidVolume(quantity);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
void GivePlantsFluidEvent::clearReferences() { m_objective.m_event.clearPointer(); }
void GivePlantsFluidEvent::onCancel()
{
	Block& location = *m_objective.m_actor.m_location;
	location.m_area->m_hasFarmFields.at(*m_objective.m_actor.getFaction()).addGivePlantFluidDesignation(m_objective.m_plantLocation->m_hasPlant.get());
}
GivePlantsFluidThreadedTask::GivePlantsFluidThreadedTask(GivePlantsFluidObjective& gpfo) : ThreadedTask(gpfo.m_actor.getThreadedTaskEngine()), m_objective(gpfo), m_plantLocation(nullptr), m_findsPath(gpfo.m_actor, gpfo.m_detour) { }
void GivePlantsFluidThreadedTask::readStep()
{
	const Faction* faction = m_objective.m_actor.getFaction();
	if(m_objective.m_plantLocation == nullptr)
	{
		std::function<bool(const Block&)> predicate = [&](const Block& block)
		{
			if(block.m_hasDesignations.contains(*faction, BlockDesignation::GivePlantFluid))
			{
				m_plantLocation = const_cast<Block*>(&block);
				return true;
			}
			return false;
		};
		// TODO: Found path is not ever used.
		m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *faction);
		if(!m_findsPath.found())
			return;
		assert(m_plantLocation != nullptr);
		assert(m_plantLocation->m_hasPlant.exists());
	}
	else if(m_objective.m_fluidHaulingItem == nullptr)
	{
		
		std::function<bool(const Block&)> predicate = [&](const Block& block) { return m_objective.canGetFluidHaulingItemAt(block); };
		m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *faction);
	}
}
void GivePlantsFluidThreadedTask::writeStep()
{
	const Faction* faction = m_objective.m_actor.getFaction();
	if(m_objective.m_plantLocation == nullptr)
	{
		m_findsPath.cacheMoveCosts();
		if(m_plantLocation == nullptr)
			// No plant accessable.
			m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
		else
		{
			m_objective.m_plantLocation = m_plantLocation;
			m_objective.execute();
		}
	}
	else if(m_objective.m_fluidHaulingItem == nullptr)
	{
		if(m_findsPath.found())
		{
			if(!m_findsPath.areAllBlocksAtDestinationReservable(faction))
			{
				m_objective.m_threadedTask.create(m_objective);
				return;
			}
			Item* item = m_objective.getFluidHaulingItemAt(*m_findsPath.getBlockWhichPassedPredicate());
			if(item == nullptr || item->m_reservable.isFullyReserved(faction))
			{
				m_objective.m_threadedTask.create(m_objective);
				return;
			}
			m_findsPath.reserveBlocksAtDestination(m_objective.m_actor.m_canReserve);
			m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath());
			m_objective.select(*item);
		}
		else
			// No container avaliable.
			m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	}
}
void GivePlantsFluidThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
bool GivePlantsFluidObjectiveType::canBeAssigned(Actor& actor) const
{
	assert(actor.m_location != nullptr);
	return actor.m_location->m_area->m_hasFarmFields.hasGivePlantsFluidDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<GivePlantsFluidObjective>(actor);
}
GivePlantsFluidObjective::GivePlantsFluidObjective(Actor& a ) : Objective(Config::givePlantsFluidPriority), m_actor(a), m_plantLocation(nullptr), m_fluidHaulingItem(nullptr), m_event(m_actor.getEventSchedule()), m_threadedTask(m_actor.getThreadedTaskEngine()) { }
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute()
{
	if(m_plantLocation == nullptr || !m_plantLocation->m_hasPlant.exists() || m_plantLocation->m_hasPlant.get().m_volumeFluidRequested == 0)
	{
		m_plantLocation = nullptr;
		// Get high priority job from area.
		Plant* plant = m_actor.m_location->m_area->m_hasFarmFields.getHighestPriorityPlantForGiveFluid(*m_actor.getFaction());
		if(plant != nullptr)
		{
			select(plant->m_location);
			execute();
		}
		else
			// No high priority job found, find a suitable plant nearby.
			m_threadedTask.create(*this);
	}
	else if(m_fluidHaulingItem == nullptr)
	{
		std::function<bool(const Block&)> predicate = [&](const Block& block) { return canGetFluidHaulingItemAt(block); };
		Block* containerLocation = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
		if(containerLocation != nullptr)
		{
			select(*getFluidHaulingItemAt(*containerLocation));
			execute();
		}
		else
			// Find fluid hauling item.
			m_threadedTask.create(*this);
	}
	else if(m_actor.m_canPickup.isCarrying(*m_fluidHaulingItem))
	{
		// Has fluid item.
		if(m_actor.m_canPickup.getFluidVolume() != 0)
		{
			Plant& plant = m_plantLocation->m_hasPlant.get();
			assert(m_fluidHaulingItem->m_hasCargo.getFluidType() == plant.m_plantSpecies.fluidType);
			// Has fluid.
			if(m_actor.isAdjacentTo(plant.m_location))
				// At plant, begin giving.
				m_event.schedule(Config::givePlantsFluidDelaySteps, *this);
			else
				// Go to plant.
				m_actor.m_canMove.setDestinationAdjacentTo(plant.m_location, m_detour);
		}
		else
		{
			std::function<bool(const Block&)> predicate = [&](const Block& block) { return canFillAt(block); };
			Block* fillLocation = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
			if(fillLocation != nullptr)
			{
				fillContainer(*fillLocation);
				// Should clearAll be moved to before setDestination so we don't leave abandoned reservations when we repath?
				// If so we need to re-reserve fluidHaulingItem.
				m_actor.m_canReserve.clearAll();
				m_detour = false;
				execute();
			}
			else
				m_actor.m_canMove.setDestinationToUnreservedAdjacentToPredicate(predicate, m_detour);
		}
	}
	else
	{
		// Does not have fluid hauling item.
		if(m_actor.isAdjacentTo(*m_fluidHaulingItem))
		{
			// Pickup item.
			m_actor.m_canPickup.pickUp(*m_fluidHaulingItem);
			m_actor.m_canReserve.clearAll();
			m_detour = false;
			execute();
		}
		else
			m_actor.m_canMove.setDestinationAdjacentTo(*m_fluidHaulingItem, m_detour);
	}
}
void GivePlantsFluidObjective::cancel()
{
	m_event.maybeUnschedule();
	m_threadedTask.maybeCancel();
	m_fluidHaulingItemOnDestroy.maybeUnsubscribe();
	if(m_plantLocation != nullptr)
		m_plantLocation->m_area->m_hasFarmFields.at(*m_actor.getFaction()).addGivePlantFluidDesignation(m_plantLocation->m_hasPlant.get());
}
void GivePlantsFluidObjective::select(Block& block)
{
	m_plantLocation = &block;
	m_plantLocation->m_reservable.reserveFor(m_actor.m_canReserve);
	m_plantLocation->m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeGivePlantFluidDesignation(m_plantLocation->m_hasPlant.get());
}
void GivePlantsFluidObjective::select(Item& item)
{
	m_fluidHaulingItem = &item;
	item.m_reservable.reserveFor(m_actor.m_canReserve);
	std::function<void()> callback = [&]() { reset(); execute(); };
	m_fluidHaulingItemOnDestroy.subscribe(item.m_onDestroy, callback);
}
void GivePlantsFluidObjective::reset()
{
	cancel();
	m_plantLocation = nullptr;
	m_fluidHaulingItem = nullptr;
	m_actor.m_canReserve.clearAll();
}
void GivePlantsFluidObjective::fillContainer(Block& fillLocation)
{
	// TODO: delay.
	Plant& plant = m_plantLocation->m_hasPlant.get();
	Item& haulTool = m_actor.m_canPickup.getItem();
	const FluidType& fluidType = plant.m_plantSpecies.fluidType;
	Item* fillFromItem = getItemToFillFromAt(fillLocation);
	Volume volume = haulTool.m_itemType.internalVolume;
	if(fillFromItem != nullptr)
	{
		assert(fillFromItem->m_hasCargo.getFluidType() == fluidType);
		volume = std::min(volume, fillFromItem->m_hasCargo.getFluidVolume());
		fillFromItem->m_hasCargo.removeFluidVolume(volume);
	}
	else
	{
		volume = std::min(volume, fillLocation.volumeOfFluidTypeContains(fluidType));
		fillLocation.removeFluid(volume, fluidType);
	}
	assert(volume != 0);
	haulTool.m_hasCargo.add(fluidType, volume);
}
bool GivePlantsFluidObjective::canFillAt(const Block& block) const
{
	assert(m_plantLocation != nullptr);
	return block.volumeOfFluidTypeContains(m_plantLocation->m_hasPlant.get().m_plantSpecies.fluidType) != 0 || 
		const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(const_cast<Block&>(block)) != nullptr;
}
Item* GivePlantsFluidObjective::getItemToFillFromAt(Block& block)
{
	assert(m_plantLocation != nullptr);
	assert(m_fluidHaulingItem != nullptr);
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_hasCargo.getFluidType() == m_plantLocation->m_hasPlant.get().m_plantSpecies.fluidType)
				return item;
	return nullptr;
}
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(const Block& block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(const_cast<Block&>(block)) != nullptr;
}
Item* GivePlantsFluidObjective::getFluidHaulingItemAt(Block& block)
{
	assert(m_plantLocation != nullptr);
	assert(m_fluidHaulingItem == nullptr);
	for(Item* item : block.m_hasItems.getAll())
		if(item->m_itemType.canHoldFluids && m_actor.m_canPickup.canPickupAny(*item))
			return item;
	return nullptr;
}
