#include "givePlantsFluid.h"
#include "../area/area.h"
#include "../config.h"
#include "../itemType.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.h"
#include "../objective.h"
#include "../reservable.h"
#include "../simulation/simulation.h"
#include "../actors/actors.h"
#include "../designations.h"
#include "../index.h"
#include "../items/items.h"
#include "../types.h"
#include "../plants.h"
#include "../path/terrainFacade.hpp"
#include "../hasShapes.hpp"

#include <memory>
GivePlantsFluidEvent::GivePlantsFluidEvent(const Step& delay, Area& area, GivePlantsFluidObjective& gpfo, const ActorIndex& actor, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_objective(gpfo)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
}
void GivePlantsFluidEvent::execute(Simulation&, Area* area)
{
	Blocks& blocks = area->getBlocks();
	Actors& actors = area->getActors();
	PlantIndex plant = blocks.plant_get(m_objective.m_plantLocation);
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	if(!blocks.plant_exists(m_objective.m_plantLocation))
			m_objective.cancel(*area, actor);
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
		area->m_hasFarmFields.getForFaction(actors.getFaction(m_actor.getIndex(actors.m_referenceData))).addGivePlantFluidDesignation(plant);
	}
}
// Path Request.
GivePlantsFluidPathRequest::GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective, const ActorIndex& actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	adjacent = true;
	reserveDestination = true;

}
GivePlantsFluidPathRequest::GivePlantsFluidPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_objective(static_cast<GivePlantsFluidObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{ }
FindPathResult GivePlantsFluidPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors &actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	DistanceInBlocks maxRange = Config::maxRangeToSearchForHorticultureDesignations;
	if(m_objective.m_plantLocation.empty())
	{
		constexpr bool unreserved = false;
		return terrainFacade.findPathToBlockDesignation(memo, BlockDesignation::GivePlantFluid, faction, start, facing, shape, m_objective.m_detour, adjacent, unreserved, maxRange);
	}
	else
	{
		assert(!m_objective.m_fluidHaulingItem.exists());
		auto destinationCondition = [this, &area, actorIndex](const BlockIndex& block, const Facing4&) -> std::pair<bool, BlockIndex>
		{
			return {m_objective.canGetFluidHaulingItemAt(area, block, actorIndex), block};
		};
		constexpr bool useAny = true;
		return terrainFacade.findPathToConditionBreadthFirst<useAny, decltype(destinationCondition)>(destinationCondition, memo, start, facing, shape, m_objective.m_detour, adjacent, faction, maxRange);
	}
}
void GivePlantsFluidPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(actorIndex, m_objective);
		return;
	}
	Blocks& blocks = area.getBlocks();
	FactionId faction = actors.getFaction(actorIndex);
	if(m_objective.m_plantLocation.empty())
	{
		if(blocks.designation_has(result.blockThatPassedPredicate, faction, BlockDesignation::GivePlantFluid))
			m_objective.selectPlantLocation(area, result.blockThatPassedPredicate, actorIndex);
		// Either path to bucket or try again.
		m_objective.makePathRequest(area, actorIndex);
	}
	else
	{
		assert(!m_objective.m_fluidHaulingItem.exists());
		if(m_objective.canGetFluidHaulingItemAt(area, result.blockThatPassedPredicate, actorIndex))
		{
			m_objective.selectItem(area, m_objective.getFluidHaulingItemAt(area, result.blockThatPassedPredicate, actorIndex), actorIndex);
			actors.move_setPath(actorIndex, result.path);
		}
		else
			// Item which was to be selected is no longer avaliable, try again.
			m_objective.makePathRequest(area, actorIndex);
	}
}
Json GivePlantsFluidPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_objective;
	output["type"] = "give plants fluid";
	return output;
}
bool GivePlantsFluidObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	assert(actors.getLocation(actor).exists());
	return area.m_hasFarmFields.hasGivePlantsFluidDesignations(actors.getFaction(actor));
}
std::unique_ptr<Objective> GivePlantsFluidObjectiveType::makeFor(Area& area, const ActorIndex&) const
{
	return std::make_unique<GivePlantsFluidObjective>(area);
}
// Objective
GivePlantsFluidObjective::GivePlantsFluidObjective(Area& area) :
	Objective(Config::givePlantsFluidPriority), m_event(area.m_eventSchedule) { }
GivePlantsFluidObjective::GivePlantsFluidObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_plantLocation(data.contains("plantLocation") ? data["plantLocation"].get<BlockIndex>() : BlockIndex::null()),
	m_event(area.m_eventSchedule)
{
	if(data.contains("fluidHaulingItem"))
		m_fluidHaulingItem.load(data["fluidHaulingItem"], area.getItems().m_referenceData);
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
void GivePlantsFluidObjective::execute(Area& area, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	if(m_plantLocation.empty() || !blocks.plant_exists(m_plantLocation) || plants.getVolumeFluidRequested(blocks.plant_get(m_plantLocation)) == 0)
	{
		m_plantLocation.clear();
		// Get high priority job from area.
		PlantIndex plant = area.m_hasFarmFields.getHighestPriorityPlantForGiveFluid(actors.getFaction(actor));
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
		std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block) { return canGetFluidHaulingItemAt(area, block, actor); };
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
	else if(actors.canPickUp_isCarryingItem(actor, m_fluidHaulingItem.getIndex(items.m_referenceData)))
	{
		PlantIndex plant = blocks.plant_get(m_plantLocation);
		// Has fluid item.
		if(actors.canPickUp_getFluidVolume(actor) != 0)
		{
			assert(items.cargo_getFluidType(m_fluidHaulingItem.getIndex(items.m_referenceData)) == PlantSpecies::getFluidType(plants.getSpecies(plant)));
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
			std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block) { return canFillAt(area, block); };
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
		ItemIndex fluidHaulingItem = m_fluidHaulingItem.getIndex(items.m_referenceData);
		if(actors.isAdjacentToItem(actor, fluidHaulingItem))
		{
			// Pickup item.
			actors.canPickUp_pickUpItem(actor, fluidHaulingItem);
			actors.canReserve_clearAll(actor);
			m_detour = false;
			execute(area, actor);
		}
		else
			actors.move_setDestinationAdjacentToItem(actor, fluidHaulingItem, m_detour);
	}
}
void GivePlantsFluidObjective::cancel(Area& area, const ActorIndex& actor)
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
			area.m_hasFarmFields.getForFaction(actors.getFaction(actor)).addGivePlantFluidDesignation(plant);
		}
	}
}
void GivePlantsFluidObjective::selectPlantLocation(Area& area, const BlockIndex& block, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	m_plantLocation = block;
	actors.canReserve_reserveLocation(actor, m_plantLocation);
	PlantIndex plant = blocks.plant_get(m_plantLocation);
	area.m_hasFarmFields.getForFaction(actors.getFaction(actor)).removeGivePlantFluidDesignation(plant);
}
void GivePlantsFluidObjective::selectItem(Area& area, const ItemIndex& item, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	m_fluidHaulingItem.setIndex(item, area.getItems().m_referenceData);
	actors.canReserve_reserveItem(actor, item, Quantity::create(1));
}
void GivePlantsFluidObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	m_plantLocation.clear();
	m_fluidHaulingItem.clear();
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
}
void GivePlantsFluidObjective::makePathRequest(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestRecord(actor, std::make_unique<GivePlantsFluidPathRequest>(area, *this, actor));
}
void GivePlantsFluidObjective::fillContainer(Area& area, const BlockIndex& fillLocation, const ActorIndex& actor)
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
bool GivePlantsFluidObjective::canFillAt(Area& area, const BlockIndex& block) const
{
	assert(m_plantLocation.exists());
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	FluidTypeId fluidType = PlantSpecies::getFluidType(plants.getSpecies(blocks.plant_get(m_plantLocation)));
	return blocks.fluid_volumeOfTypeContains(block, fluidType) != 0 ||
		const_cast<GivePlantsFluidObjective*>(this)->getItemToFillFromAt(area, block).exists();
}
ItemIndex GivePlantsFluidObjective::getItemToFillFromAt(Area& area, const BlockIndex& block) const
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
bool GivePlantsFluidObjective::canGetFluidHaulingItemAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const
{
	return const_cast<GivePlantsFluidObjective*>(this)->getFluidHaulingItemAt(area, block, actor).exists();
}
ItemIndex GivePlantsFluidObjective::getFluidHaulingItemAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const
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
