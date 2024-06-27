#include "givePlantsFluid.h"
#include "../area.h"
#include "../config.h"
#include "../itemType.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.h"
#include "../objective.h"
#include "../reservable.h"
#include "../simulation.h"
#include "../simulation/hasItems.h"
#include "actors/actors.h"
#include "designations.h"
#include "types.h"

#include <memory>
GivePlantsFluidEvent::GivePlantsFluidEvent(Area& area, Step delay, GivePlantsFluidObjective& gpfo, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_objective(gpfo) { }
void GivePlantsFluidEvent::execute(Simulation&, Area* area)
{
	Blocks& blocks = area->getBlocks();
	if(!blocks.plant_exists(m_objective.m_plantLocation))
			m_objective.cancel(*area);
	PlantIndex plant = blocks.plant_get(m_objective.m_plantLocation);
	Actors& actors = area->getActors();
	Plants& plants = area->getPlants();
	assert(actors.canPickUp_isCarryingFluidType(m_objective.m_actor, plants.getSpecies(plant).fluidType));
	uint32_t quantity = std::min(plants.getVolumeFluidRequested(plant), actors.canPickUp_getFluidVolume(m_objective.m_actor));
	plants.addFluid(plant, quantity, actors.canPickUp_getFluidType(m_objective.m_actor));
	actors.canPickUp_removeFluidVolume(m_objective.m_actor, quantity);
	actors.objective_complete(m_objective.m_actor, m_objective);
}
void GivePlantsFluidEvent::clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
void GivePlantsFluidEvent::onCancel(Simulation&, Area* area)
{
	Blocks& blocks = area->getBlocks();
	Actors& actors = area->getActors();
	BlockIndex location = m_objective.m_plantLocation;
	if(location != BLOCK_INDEX_MAX && blocks.plant_exists(location))
	{
		PlantIndex plant = blocks.plant_get(location);
		area->m_hasFarmFields.at(*actors.getFaction(m_objective.m_actor)).addGivePlantFluidDesignation(plant);
	}
}
GivePlantsFluidPathRequest::GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective) : m_objective(objective)
{
	DistanceInBlocks maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	if(m_objective.m_plantLocation == BLOCK_INDEX_MAX)
	{
		bool unreserved = false;
		createGoAdjacentToDesignation(area, m_objective.m_actor, BlockDesignation::GivePlantFluid, m_objective.m_detour, unreserved, maxRange);
	}
	else if(m_objective.m_fluidHaulingItem == ITEM_INDEX_MAX)
	{
		
		std::function<bool(BlockIndex)> predicate = [this, &area](BlockIndex block) { return m_objective.canGetFluidHaulingItemAt(area, block); };
		bool unreserved = false;
		createGoAdjacentToCondition(area, m_objective.m_actor, predicate, m_objective.m_detour, unreserved, maxRange, BLOCK_DISTANCE_MAX);
	}
}
void GivePlantsFluidPathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(m_objective.m_actor, m_objective);
		return;
	}
	Blocks& blocks = area.getBlocks();
	Faction* faction = actors.getFaction(m_objective.m_actor);
	if(m_objective.m_plantLocation == BLOCK_INDEX_MAX)
	{
		if(blocks.designation_has(result.blockThatPassedPredicate, *faction, BlockDesignation::GivePlantFluid))
			m_objective.selectPlantLocation(area, result.blockThatPassedPredicate);
		else
			// Block which was to be selected is no longer avaliable, try again.
			m_objective.makePathRequest(area);
	}
	else
	{
		assert(m_objective.m_fluidHaulingItem == ITEM_INDEX_MAX);
		if(m_objective.canGetFluidHaulingItemAt(area, result.blockThatPassedPredicate))
			m_objective.selectItem(area, m_objective.getFluidHaulingItemAt(area, result.blockThatPassedPredicate));
		else
			// Item which was to be selected is no longer avaliable, try again.
			m_objective.makePathRequest(area);
	}
}
bool GivePlantsFluidObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	Actors& actors = area.m_actors;
	assert(actors.getLocation(actor) != BLOCK_INDEX_MAX);
	return area.m_hasFarmFields.hasGivePlantsFluidDesignations(*actors.getFaction(actor));
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Area& area, ActorIndex actor) const
{
	return std::make_unique<GivePlantsFluidObjective>(area, actor);
}
// Objective
GivePlantsFluidObjective::GivePlantsFluidObjective(Area& area, ActorIndex a ) : 
	Objective(a, Config::givePlantsFluidPriority), m_event(area.m_eventSchedule) { }
/*
GivePlantsFluidObjective::GivePlantsFluidObjective(const Json& data, DeserializationMemo& deserializationMemo) : 
	Objective(data, deserializationMemo),
	m_plantLocation(data.contains("plantLocation") ? data["plantLocation"].get<BlockIndex>() : BLOCK_INDEX_MAX),
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
*/
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute(Area& area)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	if(m_plantLocation == BLOCK_INDEX_MAX || !blocks.plant_exists(m_plantLocation) || plants.getVolumeFluidRequested(blocks.plant_get(m_plantLocation)) == 0)
	{
		m_plantLocation = BLOCK_INDEX_MAX;
		// Get high priority job from area.
		PlantIndex plant = area.m_hasFarmFields.getHighestPriorityPlantForGiveFluid(*actors.getFaction(m_actor));
		if(plant != PLANT_INDEX_MAX)
		{
			selectPlantLocation(area, plants.getLocation(plant));
			execute(area);
		}
		else
			// No high priority job found, find a suitable plant nearby.
			makePathRequest(area);
	}
	else if(m_fluidHaulingItem == ITEM_INDEX_MAX)
	{
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return canGetFluidHaulingItemAt(area, block); };
		BlockIndex containerLocation = actors.getBlockWhichIsAdjacentWithPredicate(m_actor, predicate);
		if(containerLocation != BLOCK_INDEX_MAX)
		{
			selectItem(area, getFluidHaulingItemAt(area, containerLocation));
			execute(area);
		}
		else
			// Find fluid hauling item.
			makePathRequest(area);
	}
	else if(actors.canPickUp_isCarryingItem(m_actor, m_fluidHaulingItem))
	{
		PlantIndex plant = blocks.plant_get(m_plantLocation);
		// Has fluid item.
		if(actors.canPickUp_getFluidVolume(m_actor) != 0)
		{
			assert(items.cargo_getFluidType(m_fluidHaulingItem) == plants.getSpecies(plant).fluidType);
			// Has fluid.
			if(actors.isAdjacentToPlant(m_actor, plant))
				// At plant, begin giving.
				m_event.schedule(Config::givePlantsFluidDelaySteps, *this);
			else
				// Go to plant.
				actors.move_setDestinationAdjacentToPlant(plant, m_detour);
		}
		else
		{
			std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return canFillAt(area, block); };
			BlockIndex fillLocation = actors.getBlockWhichIsAdjacentWithPredicate(m_actor, predicate);
			if(fillLocation != BLOCK_INDEX_MAX)
			{
				fillContainer(area, fillLocation);
				// Should clearAll be moved to before setDestination so we don't leave abandoned reservations when we repath?
				// If so we need to re-reserve fluidHaulingItem.
				actors.canReserve_clearAll(m_actor);
				m_detour = false;
				execute(area);
			}
			else
				actors.move_setDestinationAdjacentToFluidType(m_actor, plants.getSpecies(plant).fluidType, m_detour);
		}
	}
	else
	{
		// Does not have fluid hauling item.
		if(actors.isAdjacentToItem(m_actor, m_fluidHaulingItem))
		{
			// Pickup item.
			actors.canPickUp_pickUpItem(m_actor, m_fluidHaulingItem);
			actors.canReserve_clearAll(m_actor);
			m_detour = false;
			execute(area);
		}
		else
			actors.move_setDestinationAdjacentToItem(m_actor, m_fluidHaulingItem, m_detour);
	}
}
void GivePlantsFluidObjective::cancel(Area& area)
{
	m_event.maybeUnschedule();
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(m_actor);
	if(m_plantLocation != BLOCK_INDEX_MAX)
	{
		Blocks& blocks = area.getBlocks();
		if(blocks.plant_exists(m_plantLocation))
		{
			PlantIndex plant = blocks.plant_get(m_plantLocation);
			area.m_hasFarmFields.at(*actors.getFaction(m_actor)).addGivePlantFluidDesignation(plant);
		}
	}
}
void GivePlantsFluidObjective::selectPlantLocation(Area& area, BlockIndex block)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	m_plantLocation = block;
	actors.canReserve_reserveLocation(m_plantLocation, m_actor);
	PlantIndex plant = blocks.plant_get(m_plantLocation);
	area.m_hasFarmFields.at(*actors.getFaction(m_actor)).removeGivePlantFluidDesignation(plant);
}
void GivePlantsFluidObjective::selectItem(Area& area, ItemIndex item)
{
	Actors& actors = area.getActors();
	m_fluidHaulingItem = item;
	std::unique_ptr<DishonorCallback> callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(area, m_actor);
	actors.canReserve_reserveItem(m_fluidHaulingItem, m_actor);
}
void GivePlantsFluidObjective::reset(Area& area)
{
	cancel(area);
	m_plantLocation = BLOCK_INDEX_MAX;
	m_fluidHaulingItem = ITEM_INDEX_MAX;
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(m_actor);
}
void GivePlantsFluidObjective::makePathRequest(Area& area)
{
	area.getActors().move_pathRequestRecord(m_actor, std::make_unique<GivePlantsFluidPathRequest>(area, *this));
}
void GivePlantsFluidObjective::fillContainer(Area& area, BlockIndex fillLocation)
{
	// TODO: delay.
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	PlantIndex plant = blocks.plant_get(m_plantLocation);
	ItemIndex haulTool = actors.canPickUp_getItem(m_actor);
	const FluidType& fluidType = plants.getSpecies(plant).fluidType;
	ItemIndex fillFromItem = getItemToFillFromAt(area, fillLocation);
	Volume volume = items.getItemType(haulTool).internalVolume;
	if(fillFromItem != ITEM_INDEX_MAX)
	{
		assert(items.cargo_getFluidType(fillFromItem) == fluidType);
		volume = std::min(volume, items.cargo_getFluidVolume(fillFromItem));
		items.cargo_removeFluid(fillFromItem, volume);
	}
	else
	{
		volume = std::min(volume, blocks.fluid_volumeOfTypeContains(fillLocation, fluidType));
		blocks.fluid_remove(fillLocation, volume, fluidType);
	}
	assert(volume != 0);
	items.cargo_addFluid(haulTool, fluidType, volume);
}
bool GivePlantsFluidObjective::canFillAt(Area& area, BlockIndex block) const
{
	assert(m_plantLocation != BLOCK_INDEX_MAX);
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	const FluidType& fluidType = plants.getSpecies(blocks.plant_get(m_plantLocation)).fluidType;
	return blocks.fluid_volumeOfTypeContains(block, fluidType) != 0 || 
		const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(area, block) != ITEM_INDEX_MAX;
}
ItemIndex GivePlantsFluidObjective::getItemToFillFromAt(Area& area, BlockIndex block)
{
	assert(m_plantLocation != BLOCK_INDEX_MAX);
	assert(m_fluidHaulingItem != ITEM_INDEX_MAX);
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	const FluidType& fluidType = plants.getSpecies(blocks.plant_get(m_plantLocation)).fluidType;
	for(ItemIndex item : blocks.item_getAll(block))
		if(items.cargo_getFluidType(item) == fluidType)
				return item;
	return ITEM_INDEX_MAX;
}
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(Area& area, BlockIndex block) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(area, block) != ITEM_INDEX_MAX;
}
ItemIndex GivePlantsFluidObjective::getFluidHaulingItemAt(Area& area, BlockIndex block)
{
	assert(m_plantLocation != BLOCK_INDEX_MAX);
	assert(m_fluidHaulingItem == ITEM_INDEX_MAX);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
		if(items.getItemType(item).canHoldFluids && actors.canPickUp_anyUnencomberedItem(m_actor, item) && !items.isWorkPiece(item))
			return item;
	return ITEM_INDEX_MAX;
}
