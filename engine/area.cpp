/*
* A game map made up of Blocks arranged in a cuboid with dimensions x, y, z.
*/

#include "area.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
#include "items/items.h"
#include "plants.h"
#include "config.h"
#include "deserializationMemo.h"
#include "fire.h"
#include "fluidType.h"
#include "reference.h"
#include "simulation.h"
#include "simulation/hasAreas.h"
#include "types.h"
#include "portables.hpp"
//#include "worldforge/worldLocation.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <sys/types.h>

Area::Area(AreaId id, std::wstring n, Simulation& s, const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) :
	m_eventSchedule(s, this),
	m_hasTemperature(*this),
	m_hasTerrainFacades(*this),
	m_fires(*this),
	m_hasFarmFields(*this),
	m_hasDigDesignations(*this),
	m_hasConstructionDesignations(*this),
	m_hasStockPiles(*this),
	m_hasCraftingLocationsAndJobs(*this),
	m_hasTargetedHauling(*this),
	m_hasSleepingSpots(*this),
	m_hasWoodCuttingDesignations(*this),
	m_fluidSources(*this),
	m_hasFluidGroups(*this),
	m_hasRain(*this, s),
	m_blockDesignations(*this),
	m_octTree(x, y, z),
	m_visionRequests(*this),
	m_opacityFacade(*this),
	m_name(n),
	m_simulation(s),
	m_id(id)
{
	m_blocks = std::make_unique<Blocks>(*this, x, y, z);
	m_actors = std::make_unique<Actors>(*this);
	m_plants = std::make_unique<Plants>(*this);
	m_items = std::make_unique<Items>(*this);
	setup();
	m_opacityFacade.initalize();
	m_visionCuboids.initalize(*this);
	m_hasRain.scheduleRestart();
	if constexpr(DEBUG)
		m_loaded = true;
}
Area::Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation) :
	m_eventSchedule(simulation, this),
	m_hasTemperature(*this), m_hasTerrainFacades(*this),
	m_fires(*this), m_hasFarmFields(*this), m_hasDigDesignations(*this), m_hasConstructionDesignations(*this),
	m_hasStockPiles(*this), m_hasCraftingLocationsAndJobs(*this), m_hasTargetedHauling(*this), m_hasSleepingSpots(*this),
	m_hasWoodCuttingDesignations(*this), m_fluidSources(*this), m_hasFluidGroups(*this), m_hasRain(*this, simulation), m_blockDesignations(*this),
	m_octTree(data["blocks"]["x"].get<DistanceInBlocks>(), data["blocks"]["y"].get<DistanceInBlocks>(), data["blocks"]["z"].get<DistanceInBlocks>()), m_visionRequests(*this), m_opacityFacade(*this),
	m_name(data["name"].get<std::wstring>()), m_simulation(simulation), m_id(data["id"].get<AreaId>())
{
	m_blocks = std::make_unique<Blocks>(*this, data["blocks"]["x"].get<DistanceInBlocks>(), data["blocks"]["y"].get<DistanceInBlocks>(), data["blocks"]["z"].get<DistanceInBlocks>());
	m_actors = std::make_unique<Actors>(*this);
	m_plants = std::make_unique<Plants>(*this);
	m_items = std::make_unique<Items>(*this);
	// Record id now so json block references will function later in this method.
	m_simulation.m_hasAreas->recordId(*this);
	setup();
	m_blocks->load(data["blocks"], deserializationMemo);
	m_opacityFacade.initalize();
	m_hasFluidGroups.clearMergedFluidGroups();
	data["visionCuboids"].get_to(m_visionCuboids);
	// Load fires.
	m_fires.load(data["fires"], deserializationMemo);
	// Load plants.
	m_plants->load(data["plants"]);
	// Load fields.
	m_hasFarmFields.load(data["hasFarmFields"], deserializationMemo);
	// Load Items.
	m_items->load(data["items"]);
	// Load Actors.
	m_actors->load(data["actors"]);
	// Load designations.
	m_blockDesignations.load(data["designations"], deserializationMemo);
	// Load Projects
	m_hasConstructionDesignations.load(data["hasConstructionDesignations"], deserializationMemo, *this);
	m_hasDigDesignations.load(data["hasDigDesignations"], deserializationMemo, *this);
	m_hasCraftingLocationsAndJobs.load(data["hasCraftingLocationsAndJobs"], deserializationMemo);
	m_hasStockPiles.load(data["hasStockPiles"], deserializationMemo);
	m_hasWoodCuttingDesignations.load(data["hasWoodCuttingDesignations"], deserializationMemo);
	m_hasTargetedHauling.load(data["targetedHauling"], deserializationMemo, *this);
	// Load sleeping spots.
	m_hasSleepingSpots.load(data["sleepingSpots"], deserializationMemo);
	// Load Item cargo and projects.
	m_items->loadCargoAndCraftJobs(data["items"]);
	// Load Actor objectives, following and reservations.
	m_actors->loadObjectivesAndReservations(data["actors"]);
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
		m_caveInCheck.add(blockReference.get<BlockIndex>());
	// Load rain.
	if(data.contains("rain"))
		m_hasRain.load(data["rain"], deserializationMemo);
	if constexpr(DEBUG)
		m_loaded = true;
}
Area::~Area()
{
	// Explicitly clear event schedule, threaded tasks, and path requests before destructing data tables because they have custom destructors which remove their references. These destructors must be called before the things they refer to are destroyed.
	m_eventSchedule.clear();
	// Threaded task engine needs to have simulation and area passed while event schedule does not because event schedule stores those references.
	m_threadedTaskEngine.clear(m_simulation, this);
	m_hasTerrainFacades.clearPathRequests();
}
Json Area::toJson() const
{
	Json data{
		{"id", m_id}, {"name", m_name},
		{"actors", m_actors->toJson()}, {"items", m_items->toJson()}, {"blocks", m_blocks->toJson()},
		{"plants", m_plants->toJson()}, {"fluidSources", m_fluidSources.toJson()}, {"fires", m_fires.toJson()},
		{"sleepingSpots", m_hasSleepingSpots.toJson()}, {"caveInCheck", Json::array()}, {"rain", m_hasRain.toJson()},
		{"designations", m_blockDesignations.toJson()}
	};
	data["hasFarmFields"] = m_hasFarmFields.toJson();
	data["hasSleepingSpots"] = m_hasSleepingSpots.toJson();
	data["hasConstructionDesignations"] = m_hasConstructionDesignations.toJson();
	data["hasDigDesignations"] = m_hasDigDesignations.toJson();
	data["hasWoodCuttingDesignations"] = m_hasWoodCuttingDesignations.toJson();
	data["hasCraftingLocationsAndJobs"] = m_hasCraftingLocationsAndJobs.toJson();
	data["hasStockPiles"] = m_hasStockPiles.toJson();
	data["targetedHauling"] = m_hasTargetedHauling.toJson();
	data["visionCuboids"] = m_visionCuboids;
	for(BlockIndex block : m_caveInCheck)
		data["caveInCheck"].push_back(block);
	m_opacityFacade.validate();
	return data;
}
void Area::setup()
{
	updateClimate();
}
void Area::doStep()
{
	m_hasFluidGroups.doStep();
	doStepCaveIn();
	m_visionCuboids.clearDestroyed(*this);
	m_hasTemperature.doStep();
	if(m_hasRain.isRaining() && m_simulation.m_step.modulusIsZero(Config::rainWriteStepFreqency))
		m_hasRain.doStep();
	m_fluidSources.doStep();
	m_visionRequests.doStep();
	m_hasTerrainFacades.doStep();
	m_threadedTaskEngine.doStep(m_simulation, this);
	m_eventSchedule.doStep(m_simulation.m_step);
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
	if(m_simulation.m_step.modulusIsZero(Config::stepsPerDay))
	{
		uint16_t day = DateTime(m_simulation.m_step).day;
		Plants& plants  = getPlants();
		for(auto plant : plants.getAll())
			plants.setDayOfYear(plant, day);
		m_hasFarmFields.setDayOfYear(day);
	}
}
std::wstring Area::toS() const
{
	return m_hasFluidGroups.toS();
}
void Area::logActorsAndItems() const
{
	const Actors& actors = getActors();
	const Items& items = getItems();
	for(ActorIndex actor : actors.getAll())
		actors.log(actor);
	for(ItemIndex item : items.getOnSurface())
		items.log(item);
}
Quantity Area::getTotalCountOfItemTypeOnSurface(const ItemTypeId& itemType) const
{
	const Items& items = getItems();
	Quantity output = Quantity::create(0);
	for(ItemIndex item : items.getOnSurface())
		if(itemType == items.getItemType(item))
			output += items.getQuantity(item);
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