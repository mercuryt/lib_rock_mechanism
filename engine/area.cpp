/*
* A game map.
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
	m_blocks(x*y*z), m_id(id), m_name(n), m_simulation(s), m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_hasTemperature(*this), m_hasActors(*this), m_hasFarmFields(*this), m_hasStockPiles(*this), m_hasItems(*this), m_fluidSources(*this), m_hasFluidGroups(*this), m_hasRain(*this), m_visionCuboidsActive(false)
{ 
	setup(); 
	m_hasRain.scheduleRestart();
}
Area::Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s) : 
	m_blocks(data["sizeX"].get<DistanceInBlocks>() * data["sizeY"].get<DistanceInBlocks>() * data["sizeZ"].get<DistanceInBlocks>()),
	m_id(data["id"].get<AreaId>()), m_name(data["name"].get<std::wstring>()), m_simulation(s),
	m_sizeX(data["sizeX"].get<DistanceInBlocks>()), m_sizeY(data["sizeY"].get<DistanceInBlocks>()), m_sizeZ(data["sizeZ"].get<DistanceInBlocks>()), 
	m_hasTemperature(*this), m_hasActors(*this), m_hasFarmFields(*this), m_hasStockPiles(*this), m_hasItems(*this), m_fluidSources(*this), m_hasFluidGroups(*this), m_hasRain(*this), m_visionCuboidsActive(false)
{
	// Record id now so json block references will function later in this method.
	m_simulation.m_hasAreas->recordId(*this);
	setup();
	// Load blocks.
	DistanceInBlocks x = 0;
	DistanceInBlocks y = 0;
	DistanceInBlocks z = 0;
	for(const Json& blockData : data["blocks"])
	{
		getBlock(x, y, z).loadFromJson(blockData, deserializationMemo, x, y, z);
		if(++x == m_sizeX)
		{
			x = 0;
			if(++y == m_sizeY)
			{
				y = 0;
				++z;
			}
		}
	}
	m_hasFluidGroups.clearMergedFluidGroups();
	m_hasActors.m_opacityFacade.initalize();
	// Load fires.
	m_fires.load(data["fires"], deserializationMemo);
	// Load plants.
	for(const Json& plant : data["plants"])
	{
		Block& location = deserializationMemo.blockReference(plant["location"]);
		if(location.m_hasPlant.exists())
			std::cout << " multiple plants found at block " << plant["location"];
		else
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
	/*
	hasWoodCuttingDesignations.loadWorkers(data["hasWoodCuttingDesignations"], deserializationMemo);
	m_targetedHauling.loadWorkers(data["targetedHauling"], deserializationMemo);
	*/

	// Load fluid sources.
	m_fluidSources.load(data["fluidSources"], deserializationMemo);
	// Load caveInCheck
	for(const Json& blockReference : data["caveInCheck"])
		m_caveInCheck.insert(&deserializationMemo.blockReference(blockReference));
	// Load rain.
	if(data.contains("rain"))
		m_hasRain.load(data["rain"], deserializationMemo);
	// Active vision cuboids.
	if(Config::visionCuboidsActive && data.contains("visionCuboidsActive"))
		visionCuboidsActivate();
}
Json Area::toJson() const
{
	Json data{
		{"id", m_id}, {"name", m_name}, {"sizeX", m_sizeX}, {"sizeY", m_sizeY}, {"sizeZ", m_sizeZ}, 
		{"actors", Json::array()}, {"items", Json::array()}, {"blocks", Json::array()},
		{"plants", Json::array()}, {"fluidSources", m_fluidSources.toJson()}, {"fires", m_fires.toJson()},
		{"sleepingSpots", m_hasSleepingSpots.toJson()}, {"caveInCheck", Json::array()}, {"rain", m_hasRain.toJson()}
	};
	std::unordered_set<const Item*> items;
	std::function<void(const Item&)> recordItemAndCargoItemsRecursive = [&](const Item& item){
		items.insert(&item);
		for(const Item* cargo : item.m_hasCargo.getItems())
			recordItemAndCargoItemsRecursive(*cargo);
	};
	for(const Block& block : m_blocks)
	{
		data["blocks"].push_back(block.toJson());
		for(Item* item : block.m_hasItems.getAll())
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
	if(m_visionCuboidsActive)
		data["visionCuboidsActive"] = true;
	for(const Block* block : m_caveInCheck)
		data["caveInCheck"].push_back(block);
	m_hasActors.m_opacityFacade.validate();
	return data;
}
void Area::setup()
{
	// build m_blocks
	for(DistanceInBlocks x = 0; x < m_sizeX; ++x)
		for(DistanceInBlocks y = 0; y < m_sizeY; ++y)
			for(DistanceInBlocks z = 0; z < m_sizeZ; ++z)
				getBlock(x, y, z).setup(*this, x, y, z);
	//TODO: Can we decrease cache misses by sorting blocks by distance from center?
	// record adjacent blocks
	for(DistanceInBlocks x = 0; x < m_sizeX; ++x)
		for(DistanceInBlocks y = 0; y < m_sizeY; ++y)
			for(DistanceInBlocks z = 0; z < m_sizeZ; ++z)
				getBlock(x, y, z).recordAdjacent();
	updateClimate();
	m_hasActors.m_opacityFacade.initalize();
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
	if(m_visionCuboidsActive)
		std::erase_if(m_visionCuboids, [](VisionCuboid& visionCuboid){ return visionCuboid.m_destroy; });
	// Apply vision.
	m_hasActors.processVisionWriteStep();
	// Apply temperature deltas.
	m_hasTemperature.applyDeltas();
	// Apply rain.
	if(m_simulation.m_step % Config::rainWriteStepFreqency == 0)
		m_hasRain.writeStep();
	// Apply fluid Sources.
	m_fluidSources.step();
}
Block& Area::getBlock(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z)
{
	return getBlock({x,y,z});
}
Block& Area::getBlock(Point3D coordinates)
{
	return m_blocks[getBlockIndex(coordinates)];
}
size_t Area::getBlockIndex(Point3D coordinates) const
{
	// TODO: Profile using a space filling curve such as Gilbert.
	// https://github.com/jakubcerveny/gilbert/blob/master/ports/gilbert.c
	// Ideally we would use Z-order ( Morton ) ordering but would require areas are cubes and edges are powers of 2.
	// Maybe reconsider after optimizing blocks for memory size.
	assert(coordinates.x < m_sizeX);
	assert(coordinates.y < m_sizeY);
	assert(coordinates.z < m_sizeZ);
	return coordinates.x + (coordinates.y * m_sizeX) + (coordinates.z * m_sizeY * m_sizeX); 
}
size_t Area::getBlockIndex(const Block& block) const
{
	assert(!m_blocks.empty());
	return &block - &m_blocks.front();
}
Point3D Area::getCoordinatesForIndex(BlockIndex index) const
{
	DistanceInBlocks z = index / (m_sizeX * m_sizeY);
	index -= z * m_sizeX * m_sizeY;
	DistanceInBlocks y = index / m_sizeX;
	index -= y * m_sizeX;
	DistanceInBlocks x = index;
	return {x, y, z};
}
int Area::indexOffsetForAdjacentOffset(uint8_t adjacentOffset) const
{
	auto coords = Block::offsetsListAllAdjacent[adjacentOffset];
	return indexOffsetForCoordinateOffset(coords[0], coords[1], coords[2]);
}
int Area::indexOffsetForCoordinateOffset(int x, int y, int z) const
{
	return x + (m_sizeX * y) + (m_sizeX * m_sizeY * z);
}
Facing Area::getFacingForAdjacentOffset(uint8_t adjacentOffset) const
{
	switch(adjacentOffset)
	{
		case 6:
		case 7:
		case 8:
		case 12:
		case 14:
		case 15:
		case 23:
		case 24:
		case 25:
			return 3;
		case 0:
		case 1:
		case 2:
		case 9:
		case 10:
		case 16:
			return 1;
		case 5:
		case 11:
		case 19:
		case 22:
			return 2;
		default:
			return 0;

	}
}
Block& Area::getGroundLevel(DistanceInBlocks x, DistanceInBlocks y)
{
	Block* block = &getBlock(x, y, m_sizeZ - 1);
	while(!block->isSolid())
	{
		if(!block->getBlockBelow())
			return *block;
		block = block->getBlockBelow();
	}
	return block->getBlockAbove() ? *block->getBlockAbove() : *block;
}
Block& Area::getMiddleAtGroundLevel()
{
	DistanceInBlocks x = (m_sizeX - 1) / 2;
	DistanceInBlocks y = (m_sizeY - 1) / 2;
	return getGroundLevel(x, y);
}
/*
Block& Area::getBlockForAdjacentLocation(WorldLocation& location)
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
void Area::visionCuboidsActivate()
{
	assert(!m_visionCuboidsActive);
	m_visionCuboidsActive = true;
	VisionCuboid::setup(*this);
}
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
Cuboid Area::getZLevel(DistanceInBlocks z)
{
	return Cuboid(getBlock(m_sizeX - 1, m_sizeY - 1, z), getBlock(0, 0, z));
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
