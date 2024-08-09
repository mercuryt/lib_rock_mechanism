#include "givePlantsFluid.h"
#include "../area.h"
#include "../config.h"
#include "../itemType.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.h"
#include "../objective.h"
#include "../reservable.h"
#include "../simulation.h"
#include "actors/actors.h"
#include "designations.h"
#include "index.h"
#include "items/items.h"
#include "types.h"
#include "../plants.h"

#include <memory>
GivePlantsFluidEvent::GivePlantsFluidEvent(Step delay, Area& area, GivePlantsFluidObjective& gpfo, ActorIndex actor, const Step start) : 
	ScheduledEvent(area.m_simulation, delay, start), m_objective(gpfo) 
{ 
	m_actor.setTarget(area.getActors().getReferenceTarget(actor));
}
void GivePlantsFluidEvent::execute(Simulation&, Area* area)
{
	Blocks& blocks = area->getBlocks();
	ActorIndex actor = m_actor.getIndex();
	if(!blocks.plant_exists(m_objective.m_plantLocation))
			m_objective.cancel(*area, m_actor.getIndex());
	PlantIndex plant = blocks.plant_get(m_objective.m_plantLocation);
	Actors& actors = area->getActors();
	Plants& plants = area->getPlants();
	assert(actors.canPickUp_isCarryingFluidType(actor, PlantSpecies::getFluidType(plants.getSpecies(plant))));
	CollisionVolume quantity = std::min(plants.getVolumeFluidRequested(plant), actors.canPickUp_getFluidVolume(actor));
	plants.addFluid(plant, quantity, actors.canPickUp_getFluidType(actor));
	actors.canPickUp_removeFluidVolume(actor, quantity);
	actors.objective_complete(actor, m_objective);
}
void GivePlantsFluidEvent::clearReferences(Simulation&, Area*) { m_objective.m_event.clearPointer(); }
void GivePlantsFluidEvent::onCancel(Simulation&, Area* area)
{
	Blocks& blocks = area->getBlocks();
	Actors& actors = area->getActors();
	BlockIndex location = m_objective.m_plantLocation;
	if(location.exists() && blocks.plant_exists(location))
	{
		PlantIndex plant = blocks.plant_get(location);
		area->m_hasFarmFields.at(actors.getFactionId(m_actor.getIndex())).addGivePlantFluidDesignation(plant);
	}
}
GivePlantsFluidPathRequest::GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective) : m_objective(objective)
{
	DistanceInBlocks maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	ActorIndex actor = getActor();
	if(m_objective.m_plantLocation.empty())
	{
		bool unreserved = false;
		createGoAdjacentToDesignation(area, actor, BlockDesignation::GivePlantFluid, m_objective.m_detour, unreserved, maxRange);
	}
	else if(!m_objective.m_fluidHaulingItem.exists())
	{
		
		std::function<bool(BlockIndex)> predicate = [this, &area, actor](BlockIndex block) { return m_objective.canGetFluidHaulingItemAt(area, block, actor); };
		bool unreserved = false;
		createGoAdjacentToCondition(area, actor, predicate, m_objective.m_detour, unreserved, maxRange, BlockIndex::null());
	}
}
void GivePlantsFluidPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(actor, m_objective);
		return;
	}
	Blocks& blocks = area.getBlocks();
	FactionId faction = actors.getFactionId(actor);
	if(m_objective.m_plantLocation.empty())
	{
		if(blocks.designation_has(result.blockThatPassedPredicate, faction, BlockDesignation::GivePlantFluid))
			m_objective.selectPlantLocation(area, result.blockThatPassedPredicate, actor);
		else
			// Block which was to be selected is no longer avaliable, try again.
			m_objective.makePathRequest(area, actor);
	}
	else
	{
		assert(!m_objective.m_fluidHaulingItem.exists());
		if(m_objective.canGetFluidHaulingItemAt(area, result.blockThatPassedPredicate, actor))
			m_objective.selectItem(area, m_objective.getFluidHaulingItemAt(area, result.blockThatPassedPredicate, actor), actor);
		else
			// Item which was to be selected is no longer avaliable, try again.
			m_objective.makePathRequest(area, actor);
	}
}
bool GivePlantsFluidObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	Actors& actors = area.getActors();
	assert(actors.getLocation(actor).exists());
	return area.m_hasFarmFields.hasGivePlantsFluidDesignations(actors.getFactionId(actor));
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Area& area, ActorIndex) const
{
	return std::make_unique<GivePlantsFluidObjective>(area);
}
// Objective
GivePlantsFluidObjective::GivePlantsFluidObjective(Area& area) : 
	Objective(Config::givePlantsFluidPriority), m_event(area.m_eventSchedule) { }
GivePlantsFluidObjective::GivePlantsFluidObjective(const Json& data, Area& area, ActorIndex actor) : 
	Objective(data),
	m_plantLocation(data.contains("plantLocation") ? data["plantLocation"].get<BlockIndex>() : BlockIndex::null()),
	m_event(area.m_eventSchedule)
{
	if(data.contains("fluidHaulingItem"))
		m_fluidHaulingItem.setTarget(area.getItems().getReferenceTarget(data["fluidHaulingItem"].get<ItemIndex>()));
	if(data.contains("eventStart"))
		m_event.schedule(Config::givePlantsFluidDelaySteps, area, *this, actor, data["eventStart"].get<Step>());
}
Json GivePlantsFluidObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_plantLocation.exists())
		data["plantLocation"] = m_plantLocation;
	if(m_fluidHaulingItem.exists())
		data["fluidHaulingItem"] = m_fluidHaulingItem;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
// Either get plant from Area::m_hasFarmFields or get the the nearest candidate.
// This method and GivePlantsFluidThreadedTask are complimentary state machines, with this one handling syncronus tasks.
// TODO: multi block actors.
void GivePlantsFluidObjective::execute(Area& area, ActorIndex actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	if(m_plantLocation.empty() || !blocks.plant_exists(m_plantLocation) || plants.getVolumeFluidRequested(blocks.plant_get(m_plantLocation)) == 0)
	{
		m_plantLocation.clear();
		// Get high priority job from area.
		PlantIndex plant = area.m_hasFarmFields.getHighestPriorityPlantForGiveFluid(actors.getFactionId(actor));
		if(plant.exists())
		{
			selectPlantLocation(area, plants.getLocation(plant), actor);
			execute(area, actor);
		}
		else
			// No high priority job found, find a suitable plant nearby.
			makePathRequest(area, actor);
	}
	else if(!m_fluidHaulingItem.exists())
	{
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return canGetFluidHaulingItemAt(area, block, actor); };
		BlockIndex containerLocation = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(containerLocation.exists())
		{
			selectItem(area, getFluidHaulingItemAt(area, containerLocation, actor), actor);
			execute(area, actor);
		}
		else
			// Find fluid hauling item.
			makePathRequest(area, actor);
	}
	else if(actors.canPickUp_isCarryingItem(actor, m_fluidHaulingItem.getIndex()))
	{
		PlantIndex plant = blocks.plant_get(m_plantLocation);
		// Has fluid item.
		if(actors.canPickUp_getFluidVolume(actor) != 0)
		{
			assert(items.cargo_getFluidType(m_fluidHaulingItem.getIndex()) == PlantSpecies::getFluidType(plants.getSpecies(plant)));
			// Has fluid.
			if(actors.isAdjacentToPlant(actor, plant))
				// At plant, begin giving.
				m_event.schedule(Config::givePlantsFluidDelaySteps, area, *this, actor);
			else
				// Go to plant.
				actors.move_setDestinationAdjacentToPlant(actor, plant, m_detour);
		}
		else
		{
			std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return canFillAt(area, block); };
			BlockIndex fillLocation = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
			if(fillLocation.exists())
			{
				fillContainer(area, fillLocation, actor);
				// Should clearAll be moved to before setDestination so we don't leave abandoned reservations when we repath?
				// If so we need to re-reserve fluidHaulingItem.
				actors.canReserve_clearAll(actor);
				m_detour = false;
				execute(area, actor);
			}
			else
				actors.move_setDestinationAdjacentToFluidType(actor, PlantSpecies::getFluidType(plants.getSpecies(plant)), m_detour);
		}
	}
	else
	{
		// Does not have fluid hauling item.
		if(actors.isAdjacentToItem(actor, m_fluidHaulingItem.getIndex()))
		{
			// Pickup item.
			actors.canPickUp_pickUpItem(actor, m_fluidHaulingItem.getIndex());
			actors.canReserve_clearAll(actor);
			m_detour = false;
			execute(area, actor);
		}
		else
			actors.move_setDestinationAdjacentToItem(actor, m_fluidHaulingItem.getIndex(), m_detour);
	}
}
void GivePlantsFluidObjective::cancel(Area& area, ActorIndex actor)
{
	m_event.maybeUnschedule();
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	if(m_plantLocation.exists())
	{
		Blocks& blocks = area.getBlocks();
		if(blocks.plant_exists(m_plantLocation))
		{
			PlantIndex plant = blocks.plant_get(m_plantLocation);
			area.m_hasFarmFields.at(actors.getFactionId(actor)).addGivePlantFluidDesignation(plant);
		}
	}
}
void GivePlantsFluidObjective::selectPlantLocation(Area& area, BlockIndex block, ActorIndex actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	m_plantLocation = block;
	actors.canReserve_reserveLocation(actor, m_plantLocation);
	PlantIndex plant = blocks.plant_get(m_plantLocation);
	area.m_hasFarmFields.at(actors.getFactionId(actor)).removeGivePlantFluidDesignation(plant);
}
void GivePlantsFluidObjective::selectItem(Area& area, ItemIndex item, ActorIndex actor)
{
	Actors& actors = area.getActors();
	m_fluidHaulingItem.setTarget(area.getItems().getReferenceTarget(item));
	std::unique_ptr<DishonorCallback> callback = std::make_unique<CannotCompleteObjectiveDishonorCallback>(area, actor);
	actors.canReserve_reserveItem(actor, m_fluidHaulingItem.getIndex());
}
void GivePlantsFluidObjective::reset(Area& area, ActorIndex actor)
{
	cancel(area, actor);
	m_plantLocation.clear();
	m_fluidHaulingItem.clear();
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
}
void GivePlantsFluidObjective::makePathRequest(Area& area, ActorIndex actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<GivePlantsFluidPathRequest>(area, *this));
}
void GivePlantsFluidObjective::fillContainer(Area& area, BlockIndex fillLocation, ActorIndex actor)
{
	// TODO: delay.
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	PlantIndex plant = blocks.plant_get(m_plantLocation);
	ItemIndex haulTool = actors.canPickUp_getItem(actor);
	FluidTypeId fluidType = PlantSpecies::getFluidType(plants.getSpecies(plant));
	ItemIndex fillFromItem = getItemToFillFromAt(area, fillLocation);
	CollisionVolume volume = ItemType::getInternalVolume(items.getItemType(haulTool)).toCollisionVolume();
	if(fillFromItem.exists())
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
	assert(m_plantLocation.exists());
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	FluidTypeId fluidType = PlantSpecies::getFluidType(plants.getSpecies(blocks.plant_get(m_plantLocation)));
	return blocks.fluid_volumeOfTypeContains(block, fluidType) != 0 || 
		const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(area, block).exists();
}
ItemIndex GivePlantsFluidObjective::getItemToFillFromAt(Area& area, BlockIndex block)
{
	assert(m_plantLocation.exists());
	assert(m_fluidHaulingItem.exists());
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	FluidTypeId fluidType = PlantSpecies::getFluidType(plants.getSpecies(blocks.plant_get(m_plantLocation)));
	for(ItemIndex item : blocks.item_getAll(block))
		if(items.cargo_getFluidType(item) == fluidType)
				return item;
	return ItemIndex::null();
}
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(Area& area, BlockIndex block, ActorIndex actor) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(area, block, actor).exists();
}
ItemIndex GivePlantsFluidObjective::getFluidHaulingItemAt(Area& area, BlockIndex block, ActorIndex actor)
{
	assert(m_plantLocation.exists());
	assert(!m_fluidHaulingItem.exists());
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
		if(ItemType::getCanHoldFluids(items.getItemType(item)) && actors.canPickUp_itemUnencombered(actor, item) && !items.isWorkPiece(item))
			return item;
	return ItemIndex::null();
}
