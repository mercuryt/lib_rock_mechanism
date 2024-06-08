#include "givePlantsFluid.h"
#include "../item.h"
#include "../area.h"
#include "../actor.h"
#include "../item.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.h"
#include "../objective.h"
#include "../reservable.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "types.h"

#include <memory>
GivePlantsFluidEvent::GivePlantsFluidEvent(Step delay, GivePlantsFluidObjective& gpfo, const Step start) : 
	ScheduledEvent(gpfo.m_actor.getSimulation(), delay, start), m_objective(gpfo) { }
void GivePlantsFluidEvent::execute()
{
	auto& blocks = m_objective.m_actor.m_area->getBlocks();
	if(!blocks.plant_exists(m_objective.m_plantLocation))
			m_objective.cancel();
	Plant& plant = blocks.plant_get(m_objective.m_plantLocation);
	assert(m_objective.m_actor.m_canPickup.isCarryingFluidType(plant.m_plantSpecies.fluidType));
	uint32_t quantity = std::min(plant.getVolumeFluidRequested(), m_objective.m_actor.m_canPickup.getFluidVolume());
	plant.addFluid(quantity, m_objective.m_actor.m_canPickup.getFluidType());
	m_objective.m_actor.m_canPickup.removeFluidVolume(quantity);
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
void GivePlantsFluidEvent::clearReferences() { m_objective.m_event.clearPointer(); }
void GivePlantsFluidEvent::onCancel()
{
	BlockIndex location = m_objective.m_actor.m_location;
	auto& blocks = m_objective.m_actor.m_area->getBlocks();
	if(blocks.plant_exists(location))
	{
		Plant& plant = blocks.plant_get(location);
		m_objective.m_actor.m_area->m_hasFarmFields.at(*m_objective.m_actor.getFaction()).addGivePlantFluidDesignation(plant);
	}
}
GivePlantsFluidThreadedTask::GivePlantsFluidThreadedTask(GivePlantsFluidObjective& gpfo) : 
	ThreadedTask(gpfo.m_actor.getThreadedTaskEngine()), m_objective(gpfo), m_findsPath(gpfo.m_actor, gpfo.m_detour) { }
void GivePlantsFluidThreadedTask::readStep()
{
	Faction* faction = m_objective.m_actor.getFaction();
	m_findsPath.m_maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	if(m_objective.m_plantLocation == BLOCK_INDEX_MAX)
	{
		auto& blocks = m_objective.m_actor.m_area->getBlocks();
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block)
		{
			if(blocks.designation_has(block, *faction, BlockDesignation::GivePlantFluid))
			{
				m_plantLocation = block;
				return true;
			}
			return false;
		};
		// TODO: Found path is not ever used.
		m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *faction);
		if(!m_findsPath.found())
			return;
		assert(m_plantLocation != BLOCK_INDEX_MAX);
		assert(blocks.plant_exists(m_plantLocation));
	}
	else if(m_objective.m_fluidHaulingItem == nullptr)
	{
		
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return m_objective.canGetFluidHaulingItemAt(block); };
		m_findsPath.pathToUnreservedAdjacentToPredicate(predicate, *faction);
	}
}
void GivePlantsFluidThreadedTask::writeStep()
{
	Faction* faction = m_objective.m_actor.getFaction();
	if(m_objective.m_plantLocation == BLOCK_INDEX_MAX)
	{
		if(m_plantLocation == BLOCK_INDEX_MAX)
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
		if(!m_findsPath.found() && !m_findsPath.m_useCurrentLocation)
		{
			// No container avaliable.
			m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
			return;
		}
		if(!m_findsPath.areAllBlocksAtDestinationReservable(faction))
		{
			// Selected location reserved already, try again.
			m_objective.m_threadedTask.create(m_objective);
			return;
		}
		Item* item = m_objective.getFluidHaulingItemAt(m_findsPath.getBlockWhichPassedPredicate());
		if(item == nullptr || item->m_reservable.isFullyReserved(faction))
		{
			// Selected Item moved, destroyed or reserved, try again.
			m_objective.m_threadedTask.create(m_objective);
			return;
		}
		m_objective.select(*item);
		if(m_findsPath.found())
		{
			m_findsPath.reserveBlocksAtDestination(m_objective.m_actor.m_canReserve);
			m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath());
		}
		else
		{
			m_objective.m_actor.reserveOccupied(m_objective.m_actor.m_canReserve);
			m_objective.execute();
		}
	}
}
void GivePlantsFluidThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
bool GivePlantsFluidObjectiveType::canBeAssigned(Actor& actor) const
{
	assert(actor.m_location != BLOCK_INDEX_MAX);
	return actor.m_area->m_hasFarmFields.hasGivePlantsFluidDesignations(*actor.getFaction());
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<GivePlantsFluidObjective>(actor);
}
// Objective
GivePlantsFluidObjective::GivePlantsFluidObjective(Actor& a ) : 
	Objective(a, Config::givePlantsFluidPriority), m_event(m_actor.getEventSchedule()), m_threadedTask(m_actor.getThreadedTaskEngine()) { }
GivePlantsFluidObjective::GivePlantsFluidObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo),
	m_plantLocation(data.contains("plantLocation") ? deserializationMemo.m_simulation.getBlockForJsonQuery(data["plantLocation"]) : BLOCK_INDEX_MAX),
	m_fluidHaulingItem(data.contains("fluidHaulingItem") ? &deserializationMemo.m_simulation.m_hasItems->getById(data["fluidHaulingItem"].get<ItemId>()) : nullptr),
	m_event(m_actor.getEventSchedule()), m_threadedTask(m_actor.getThreadedTaskEngine())
{
	if(data.contains("eventStart"))
		m_event.schedule(Config::givePlantsFluidDelaySteps, *this, data["eventStart"].get<Step>());
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json GivePlantsFluidObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_plantLocation != BLOCK_INDEX_MAX)
		data["plantLocation"] = m_plantLocation;
	if(m_fluidHaulingItem)
		data["fluidHaulingItem"] = m_fluidHaulingItem;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute()
{
	auto& blocks = m_actor.m_area->getBlocks();
	if(m_plantLocation == BLOCK_INDEX_MAX || !blocks.plant_exists(m_plantLocation) || blocks.plant_get(m_plantLocation).m_volumeFluidRequested == 0)
	{
		m_plantLocation = BLOCK_INDEX_MAX;
		// Get high priority job from area.
		Plant* plant = m_actor.m_area->m_hasFarmFields.getHighestPriorityPlantForGiveFluid(*m_actor.getFaction());
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
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return canGetFluidHaulingItemAt(block); };
		BlockIndex containerLocation = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
		if(containerLocation != BLOCK_INDEX_MAX)
		{
			select(*getFluidHaulingItemAt(containerLocation));
			execute();
		}
		else
			// Find fluid hauling item.
			m_threadedTask.create(*this);
	}
	else if(m_actor.m_canPickup.isCarrying(*m_fluidHaulingItem))
	{
		Plant& plant = blocks.plant_get(m_plantLocation);
		// Has fluid item.
		if(m_actor.m_canPickup.getFluidVolume() != 0)
		{
			assert(m_fluidHaulingItem->m_hasCargo.getFluidType() == plant.m_plantSpecies.fluidType);
			// Has fluid.
			if(m_actor.isAdjacentTo(plant))
				// At plant, begin giving.
				m_event.schedule(Config::givePlantsFluidDelaySteps, *this);
			else
				// Go to plant.
				m_actor.m_canMove.setDestinationAdjacentTo(plant, m_detour);
		}
		else
		{
			std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return canFillAt(block); };
			BlockIndex fillLocation = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
			if(fillLocation != BLOCK_INDEX_MAX)
			{
				fillContainer(fillLocation);
				// Should clearAll be moved to before setDestination so we don't leave abandoned reservations when we repath?
				// If so we need to re-reserve fluidHaulingItem.
				m_actor.m_canReserve.deleteAllWithoutCallback();
				m_detour = false;
				execute();
			}
			else
				m_actor.m_canMove.setDestinationAdjacentTo(plant.m_plantSpecies.fluidType, m_detour);
		}
	}
	else
	{
		// Does not have fluid hauling item.
		if(m_actor.isAdjacentTo(*m_fluidHaulingItem))
		{
			// Pickup item.
			m_actor.m_canPickup.pickUp(*m_fluidHaulingItem);
			m_actor.m_canReserve.deleteAllWithoutCallback();
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
	if(m_plantLocation != BLOCK_INDEX_MAX)
	{
		auto& blocks = m_actor.m_area->getBlocks();
		if(blocks.plant_exists(m_plantLocation))
		{
			Plant& plant = blocks.plant_get(m_plantLocation);
			m_actor.m_area->m_hasFarmFields.at(*m_actor.getFaction()).addGivePlantFluidDesignation(plant);
		}
	}
}
void GivePlantsFluidObjective::select(BlockIndex block)
{
	auto& blocks = m_actor.m_area->getBlocks();
	m_plantLocation = block;
	blocks.reserve(m_plantLocation, m_actor.m_canReserve);
	Plant& plant = blocks.plant_get(m_plantLocation);
	m_actor.m_area->m_hasFarmFields.at(*m_actor.getFaction()).removeGivePlantFluidDesignation(plant);
}
void GivePlantsFluidObjective::select(Item& item)
{
	m_fluidHaulingItem = &item;
	std::unique_ptr<DishonorCallback> callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(m_actor);
	item.m_reservable.reserveFor(m_actor.m_canReserve, 1u, std::move(callback));
}
void GivePlantsFluidObjective::reset()
{
	cancel();
	m_plantLocation = BLOCK_INDEX_MAX;
	m_fluidHaulingItem = nullptr;
	m_actor.m_canReserve.deleteAllWithoutCallback();
}
void GivePlantsFluidObjective::fillContainer(BlockIndex fillLocation)
{
	// TODO: delay.
	auto& blocks = m_actor.m_area->getBlocks();
	Plant& plant = blocks.plant_get(m_plantLocation);
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
		volume = std::min(volume, blocks.fluid_volumeOfTypeContains(fillLocation, fluidType));
		blocks.fluid_remove(fillLocation, volume, fluidType);
	}
	assert(volume != 0);
	haulTool.m_hasCargo.add(fluidType, volume);
}
bool GivePlantsFluidObjective::canFillAt(BlockIndex block) const
{
	assert(m_plantLocation != BLOCK_INDEX_MAX);
	auto& blocks = m_actor.m_area->getBlocks();
	const FluidType& fluidType = blocks.plant_get(m_plantLocation).m_plantSpecies.fluidType;
	return blocks.fluid_volumeOfTypeContains(block, fluidType) != 0 || 
		const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(block) != nullptr;
}
Item* GivePlantsFluidObjective::getItemToFillFromAt(BlockIndex block)
{
	assert(m_plantLocation != BLOCK_INDEX_MAX);
	assert(m_fluidHaulingItem != nullptr);
	auto& blocks = m_actor.m_area->getBlocks();
	const FluidType& fluidType = blocks.plant_get(m_plantLocation).m_plantSpecies.fluidType;
	for(Item* item : blocks.item_getAll(block))
		if(item->m_hasCargo.getFluidType() == fluidType)
				return item;
	return nullptr;
}
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(BlockIndex block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(block) != nullptr;
}
Item* GivePlantsFluidObjective::getFluidHaulingItemAt(BlockIndex block)
{
	assert(m_plantLocation != BLOCK_INDEX_MAX);
	assert(m_fluidHaulingItem == nullptr);
	auto& blocks = m_actor.m_area->getBlocks();
	for(Item* item : blocks.item_getAll(block))
		if(item->m_itemType.canHoldFluids && m_actor.m_canPickup.canPickupAny(*item) && !item->isWorkPiece())
			return item;
	return nullptr;
}
