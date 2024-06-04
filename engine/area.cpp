/*
* A game map made up of Blocks arranged in a cuboid with dimensions x, y, z.
*/

#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "fire.h"
#include "fluidType.h"
#include "plant.h"
#include "simulation.h"
#include "simulation/hasAreas.h"
#include "simulation/hasItems.h"
#include "simulation/hasActors.h"
#include "types.h"
//#include "worldforge/worldLocation.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <sys/types.h>
#include <unordered_set>

Area::Area(AreaId id, std::wstring n, Simulation& s, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z) :
	m_blocks(*this, x, y, z), m_hasTemperature(*this), m_hasTerrainFacades(*this), m_hasActors(*this), m_hasPlants(*this), 
	m_fires(*this), m_hasFarmFields(*this), m_hasDigDesignations(*this), m_hasConstructionDesignations(*this), 
	m_hasStockPiles(*this), m_hasCraftingLocationsAndJobs(*this), m_hasTargetedHauling(*this), m_hasSleepingSpots(*this), 
	m_hasWoodCuttingDesignations(*this), m_hasItems(*this), m_fluidSources(*this), m_hasFluidGroups(*this), 
	m_hasRain(*this, s), m_blockDesignations(*this), m_name(n), m_simulation(s), m_id(id)
{ 
	setup(); 
	m_hasActors.m_visionCuboids.initalize(*this);
	m_hasRain.scheduleRestart();
}
Area::Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s) : 
	m_blocks(*this, data["sizeX"].get<DistanceInBlocks>(), data["sizeY"].get<DistanceInBlocks>(), data["sizeZ"].get<DistanceInBlocks>()), 
	m_hasTemperature(*this), m_hasTerrainFacades(*this), m_hasActors(*this), m_hasPlants(*this), m_fires(*this), 
	m_hasFarmFields(*this), m_hasDigDesignations(*this), m_hasConstructionDesignations(*this), m_hasStockPiles(*this), 
	m_hasCraftingLocationsAndJobs(*this), m_hasTargetedHauling(*this), m_hasSleepingSpots(*this), m_hasWoodCuttingDesignations(*this), 
	m_hasItems(*this), m_fluidSources(*this), m_hasFluidGroups(*this), m_hasRain(*this, s), m_blockDesignations(*this), 
	m_name(data["name"].get<std::wstring>()), m_simulation(s), m_id(data["id"].get<AreaId>())
{
	// Record id now so json block references will function later in this method.
	m_simulation.m_hasAreas->recordId(*this);
	setup();
	m_blocks.load(data["blocks"], deserializationMemo);
	m_hasFluidGroups.clearMergedFluidGroups();
	m_hasActors.m_visionCuboids.initalize(*this);
	// Load fires.
	m_fires.load(data["fires"], deserializationMemo);
	// Load plants.
	for(const Json& plant : data["plants"])
	{
		BlockIndex location = plant["location"].get<BlockIndex>();
		assert(!m_blocks.plant_exists(location));
		m_hasPlants.emplace(plant, deserializationMemo);
	}
	// Load fields.
	m_hasFarmFields.load(data["hasFarmFields"], deserializationMemo);
	// Load Items.
	for(const Json& item : data["items"])
		m_simulation.m_hasItems->loadItemFromJson(item, deserializationMemo);
	// Load Actors.
	for(const Json& actor : data["actors"])
		m_simulation.m_hasActors->loadActorFromJson(actor, deserializationMemo);
	// Load designations.
	// Temporary shim.
	if(data.contains("designations"))
		m_blockDesignations.load(data["designations"], deserializationMemo);
	// Load Projects
	m_hasConstructionDesignations.load(data["hasConstructionDesignations"], deserializationMemo);
	m_hasDigDesignations.load(data["hasDigDesignations"], deserializationMemo);
	m_hasCraftingLocationsAndJobs.load(data["hasCraftingLocationsAndJobs"], deserializationMemo);
	m_hasStockPiles.load(data["hasStockPiles"], deserializationMemo);
	m_hasWoodCuttingDesignations.load(data["hasWoodCuttingDesignations"], deserializationMemo);
	m_hasTargetedHauling.load(data["targetedHauling"], deserializationMemo);
	// Load sleeping spots.
	m_hasSleepingSpots.load(data["sleepingSpots"], deserializationMemo);
	// Load Item cargo and projects.
	for(const Json& itemData : data["items"])
	{
		Item& item = m_simulation.m_hasItems->getById(itemData["id"].get<ItemId>());
		item.load(itemData, deserializationMemo);
	}
	// Load Actor objectives, following and reservations.
	for(const Json& actorData : data["actors"])
	{
		Actor& actor = m_simulation.m_hasActors->getById(actorData["id"].get<ActorId>());
		actor.m_hasObjectives.load(actorData["hasObjectives"], deserializationMemo);
		if(actorData.contains("canReserve"))
			actor.m_canReserve.load(actorData["canReserve"], deserializationMemo);
		if(actorData.contains("canFollow"))
			actor.m_canFollow.load(actorData["canFollow"], deserializationMemo);
	}
	// Load project workers
	m_hasConstructionDesignations.loadWorkers(data["hasConstructionDesignations"], deserializationMemo);
	m_hasDigDesignations.loadWorkers(data["hasDigDesignations"], deserializationMemo);
	m_hasStockPiles.loadWorkers(data["hasStockPiles"], deserializationMemo);
	m_hasCraftingLocationsAndJobs.loadWorkers(data["hasCraftingLocationsAndJobs"], deserializationMemo);
	
	//hasWoodCuttingDesignations.loadWorkers(data["hasWoodCuttingDesignations"], deserializationMemo);
	//m_targetedHauling.loadWorkers(data["targetedHauling"], deserializationMemo);

	// Load fluid sources.
	m_fluidSources.load(data["fluidSources"], deserializationMemo);
	// Load caveInCheck
	for(const Json& blockReference : data["caveInCheck"])
		m_caveInCheck.insert(blockReference.get<BlockIndex>());
	// Load rain.
	if(data.contains("rain"))
		m_hasRain.load(data["rain"], deserializationMemo);
}
Json Area::toJson() const
{
	Json data{
		{"id", m_id}, {"name", m_name}, 
		{"actors", Json::array()}, {"items", Json::array()}, {"blocks", m_blocks.toJson()},
		{"plants", Json::array()}, {"fluidSources", m_fluidSources.toJson()}, {"fires", m_fires.toJson()},
		{"sleepingSpots", m_hasSleepingSpots.toJson()}, {"caveInCheck", Json::array()}, {"rain", m_hasRain.toJson()},
		{"designations", m_blockDesignations.toJson()}
	};
	std::unordered_set<const Item*> items;
	std::function<void(const Item&)> recordItemAndCargoItemsRecursive = [&](const Item& item){
		items.insert(&item);
		for(const Item* cargo : item.m_hasCargo.getItems())
			recordItemAndCargoItemsRecursive(*cargo);
	};
	for(BlockIndex block : m_blocks.getAll())
	{
		for(Item* item : m_blocks.item_getAll(block))
			recordItemAndCargoItemsRecursive(*item);
	}
	for(Actor* actor : m_hasActors.getAllConst())
	{
		data["actors"].push_back(actor->toJson());
		for(Item* item : actor->m_equipmentSet.getAll())
			recordItemAndCargoItemsRecursive(*item);
		if(actor->m_canPickup.isCarryingAnything())
		{
			if(actor->m_canPickup.getCarrying()->isItem())
				recordItemAndCargoItemsRecursive(actor->m_canPickup.getItem());
			else if(actor->m_canPickup.getCarrying()->isActor())
				data["actors"].push_back(actor->m_canPickup.getActor().toJson());
		}
	}
	for(const Item* item : items)
		data["items"].push_back(item->toJson());
	for(const Plant& plant : m_hasPlants.getAllConst())
		data["plants"].push_back(plant.toJson());
	data["hasFarmFields"] = m_hasFarmFields.toJson();
	data["hasSleepingSpots"] = m_hasSleepingSpots.toJson();
	data["hasConstructionDesignations"] = m_hasConstructionDesignations.toJson();
	data["hasDigDesignations"] = m_hasDigDesignations.toJson();
	data["hasWoodCuttingDesignations"] = m_hasWoodCuttingDesignations.toJson();
	data["hasCraftingLocationsAndJobs"] = m_hasCraftingLocationsAndJobs.toJson();
	data["hasStockPiles"] = m_hasStockPiles.toJson();
	data["targetedHauling"] = m_hasTargetedHauling.toJson();
	for(BlockIndex block : m_caveInCheck)
		data["caveInCheck"].push_back(block);
	m_hasActors.m_opacityFacade.validate();
	return data;
}
void Area::setup()
{
	m_hasActors.m_locationBuckets.initalize();
	m_blocks.assignLocationBuckets();
	m_hasActors.m_opacityFacade.initalize();
	updateClimate();
}
void Area::readStep()
{ 
	//TODO: Count tasks dispatched and finished instead of pool.wait_for_tasks so we can do multiple areas simultaniously in one pool.
	// Process vision, emplace requests for every actor in current bucket.
	// It seems like having the vision requests permanantly embeded in the actors and iterating the vision bucket directly rather then using the visionRequestQueue should be faster but limited testing shows otherwise.
	m_hasActors.processVisionReadStep();
	// Calculate cave in.
	m_simulation.m_taskFutures.push_back(m_simulation.m_pool.submit([&](){ stepCaveInRead(); }));
	m_hasFluidGroups.readStep();
}
void Area::writeStep()
{ 
	m_hasFluidGroups.writeStep();
	// Apply cave in.
	if(!m_caveInData.empty())
		stepCaveInWrite();
	// Clean up old vision cuboids.
	m_hasActors.m_visionCuboids.clearDestroyed();
	// Apply vision.
	m_hasActors.processVisionWriteStep();
	// Apply temperature deltas.
	m_hasTemperature.applyDeltas();
	// Apply rain.
	if(m_hasRain.isRaining() && m_simulation.m_step % Config::rainWriteStepFreqency == 0)
		m_hasRain.writeStep();
	// Apply fluid Sources.
	m_fluidSources.step();
}
/*
BlockIndex Area::getBlockForAdjacentLocation(WorldLocation& location)
{
	if(&location == m_worldLocation->west)
		// West
		return getGroundLevel(m_sizeX - 2, (m_sizeY - 1) / 2);
	if(std::ranges::find(m_worldLocation->north, &location) != m_worldLocation->north.end())
	{
		if(*m_worldLocation->north.begin() == &location)
			// northeast
			return getGroundLevel(0,0);
		else if(*m_worldLocation->north.end() - 1 == &location)
			// northwest
			return getGroundLevel(m_sizeX - 1, 0);
		else
			// center north
			return getGroundLevel((m_sizeX - 1) / 2, 0);
	}
	if(std::ranges::find(m_worldLocation->south, &location) != m_worldLocation->south.end())
	{
		if(*m_worldLocation->south.begin() == &location)
			// southeast
			return getGroundLevel(0, m_sizeY - 1);
		else if(*m_worldLocation->south.end() - 1 == &location)
			// southwest
			return getGroundLevel(m_sizeX - 1,  m_sizeY - 1);
		else
			// center south
			return getGroundLevel((m_sizeX - 1) / 2,  m_sizeY - 1);
	}
	// East
	assert(&location == m_worldLocation->east);
	return getGroundLevel(0, (m_sizeY - 1) / 2);
}
*/
void Area::updateClimate()
{
	//TODO: daylight.
	m_hasTemperature.updateAmbientSurfaceTemperature();
	// Once per day.
	if(m_simulation.m_step % Config::stepsPerDay == 0)
	{
		uint16_t day = DateTime(m_simulation.m_step).day;
		for(Plant& plant : m_hasPlants.getAll())
			plant.setDayOfYear(day);
		m_hasFarmFields.setDayOfYear(day);
	}
}
std::string Area::toS() const
{
	return m_hasFluidGroups.toS();
}
void Area::logActorsAndItems() const
{
	for(Actor* actor : m_hasActors.getAllConst())
		actor->log();
	for(Item* item : m_hasItems.getOnSurfaceConst())
		item->log();
}
uint32_t Area::getTotalCountOfItemTypeOnSurface(const ItemType& itemType) const
{
	uint32_t output = 0;
	for(Item* item : m_hasItems.getOnSurfaceConst())
		if(itemType == item->m_itemType)
			output += item->getQuantity();
	return output;
}
void Area::clearReservations()
{
	m_hasDigDesignations.clearReservations();
	m_hasConstructionDesignations.clearReservations();
	m_hasStockPiles.clearReservations();
	m_hasCraftingLocationsAndJobs.clearReservations();
	m_hasWoodCuttingDesignations.clearReservations();
	m_hasTargetedHauling.clearReservations();
	m_hasInstallItemDesignations.clearReservations();
	//m_hasMedicalPatientsAndDoctors.clearReservations();
}
