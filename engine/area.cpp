/*
* A game map.
*/

#include "area.h"
#include "deserializationMemo.h"
#include "fire.h"
#include "plant.h"
#include "simulation.h"
//#include "worldforge/worldLocation.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <sys/types.h>
#include <unordered_set>

Area::Area(AreaId id, std::wstring n, Simulation& s, uint32_t x, uint32_t y, uint32_t z) :
	m_blocks(x*y*z), m_id(id), m_name(n), m_simulation(s), m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_hasTemperature(*this), m_hasActors(*this), m_hasFarmFields(*this), m_hasStockPiles(*this), m_hasItems(*this), m_hasRain(*this), m_visionCuboidsActive(false)
{ setup(); }
Area::Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s) : 
	m_blocks(data["sizeX"].get<uint32_t>() * data["sizeY"].get<uint32_t>() * data["sizeZ"].get<uint32_t>()),
	m_id(data["id"].get<AreaId>()), m_name(data["name"].get<std::wstring>()), m_simulation(s),
	m_sizeX(data["sizeX"].get<uint32_t>()), m_sizeY(data["sizeY"].get<uint32_t>()), m_sizeZ(data["sizeZ"].get<uint32_t>()), 
	m_hasTemperature(*this), m_hasActors(*this), m_hasFarmFields(*this), m_hasStockPiles(*this), m_hasItems(*this), m_hasRain(*this), m_visionCuboidsActive(false)
{
	setup();
	// Load blocks.
	for(const Json& blockData : data["blocks"])
		getBlock(blockData["x"].get<uint32_t>(), blockData["y"].get<uint32_t>(), blockData["z"].get<uint32_t>()).loadFromJson(blockData, deserializationMemo);
	// Load fires.
	m_fires.load(data["fires"], deserializationMemo);
	// Load plants.
	for(const Json& plant : data["plants"])
		m_hasPlants.emplace(plant, deserializationMemo);
	// Load fields.
	m_hasFarmFields.load(data["hasFarmFields"], deserializationMemo);
	// Load Items.
	for(const Json& item : data["items"])
		m_simulation.loadItemFromJson(item, deserializationMemo);
	// Load Actors.
	for(const Json& actor : data["actors"])
		m_simulation.loadActorFromJson(actor, deserializationMemo);
	// Load Projects
	m_hasConstructionDesignations.load(data["hasConstructionDesignations"], deserializationMemo);
	m_hasDigDesignations.load(data["hasDigDesignations"], deserializationMemo);
	m_hasCraftingLocationsAndJobs.load(data["hasCraftingLocationsAndJobs"], deserializationMemo);
	m_hasStockPiles.load(data["hasStockPiles"], deserializationMemo);
	m_targetedHauling.load(data["targetedHauling"], deserializationMemo);
	// Load sleeping spots.
	m_hasSleepingSpots.load(data["sleepingSpots"], deserializationMemo);
	// Load Item cargo.
	for(const Json& itemData : data["items"])
		if(itemData.contains("hasCargo"))
		{
			Item& item = m_simulation.getItemById(itemData["id"].get<ItemId>());
			item.m_hasCargo.loadJson(itemData["hasCargo"], deserializationMemo);
		}
	// Load Actor objectives and reservations.
	for(const Json& actorData : data["actors"])
	{
		Actor& actor = m_simulation.getActorById(actorData["id"].get<ActorId>());
		if(actorData.contains("hasObjectives"))
			actor.m_hasObjectives.load(actorData["hasObjectives"], deserializationMemo);
		if(actorData.contains("canFollow"))
			actor.m_canFollow.load(actorData["canFollow"], deserializationMemo);

	}
	m_fluidSources.load(data["fluidSources"], deserializationMemo);
}
Json Area::toJson() const
{
	Json data{
		{"id", m_id}, {"name", m_name}, {"sizeX", m_sizeX}, {"sizeY", m_sizeY}, {"sizeZ", m_sizeZ}, 
		{"actors", Json::array()}, {"items", Json::array()}, {"blocks", Json::array()},
		{"plants", Json::array()}, {"fluidSources", m_fluidSources.toJson()}
	};
	std::unordered_set<Item*> items;
	for(const Block& block : m_blocks)
	{
		data["blocks"].push_back(block.toJson());
		for(Item* item : items)
			items.insert(item);
	}
	for(Actor* actor : m_hasActors.getAllConst())
		data["actors"].push_back(actor->toJson());
	for(Item* item : items)
		data["items"].push_back(item->toJson());
	for(const Plant& plant : m_hasPlants.getAllConst())
		data["plants"].push_back(plant.toJson());
	data["hasFarmFields"] = m_hasFarmFields.toJson();
	data["hasSleepingSpots"] = m_hasSleepingSpots.toJson();
	data["hasConstructionDesignations"] = m_hasConstructionDesignations.toJson();
	data["hasDigDesignations"] = m_hasDigDesignations.toJson();
	data["hasCraftingLocationsAndJobs"] = m_hasCraftingLocationsAndJobs.toJson();
	data["hasStockPiles"] = m_hasStockPiles.toJson();
	data["targetedHauling"] = m_targetedHauling.toJson();
	return data;
}
void Area::setup()
{
	// build m_blocks
	for(uint32_t x = 0; x < m_sizeX; ++x)
		for(uint32_t y = 0; y < m_sizeY; ++y)
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				getBlock(x, y, z).setup(*this, x, y, z);
	//TODO: Can we decrease cache misses by sorting blocks by distance from center?
	// record adjacent blocks
	for(uint32_t x = 0; x < m_sizeX; ++x)
		for(uint32_t y = 0; y < m_sizeY; ++y)
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				getBlock(x, y, z).recordAdjacent();
	setDateTime(m_simulation.m_now);
}
void Area::readStep()
{ 
	//TODO: Count tasks dispatched and finished instead of pool.wait_for_tasks so we can do multiple areas simultaniously in one pool.
	// Process vision, emplace requests for every actor in current bucket.
	// It seems like having the vision requests permanantly embeded in the actors and iterating the vision bucket directly rather then using the visionRequestQueue should be faster but limited testing shows otherwise.
	m_hasActors.processVisionReadStep();
	// Calculate cave in.
	m_simulation.m_taskFutures.push_back(m_simulation.m_pool.submit([&](){ stepCaveInRead(); }));
	// Calculate flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		assert(!fluidGroup->m_destroy);
		m_simulation.m_taskFutures.push_back(m_simulation.m_pool.submit([=](){ fluidGroup->readStep(); }));
	}
}
void Area::writeStep()
{ 
	// Remove destroyed.
	for(FluidGroup& fluidGroup : m_fluidGroups)
		if(fluidGroup.m_destroy)
			m_unstableFluidGroups.erase(&fluidGroup);
	std::erase_if(m_fluidGroups, [](FluidGroup& fluidGroup){ return fluidGroup.m_destroy; });
	// Apply flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		fluidGroup->writeStep();
		validateAllFluidGroups();
	}
	// Resolve overfull, diagonal seep, and mist.
	// Make vector of unstable so we can iterate it while modifing the original.
	std::vector<FluidGroup*> unstable(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable)
	{
		fluidGroup->afterWriteStep();
		fluidGroup->validate();
	}
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged || fluidGroup->m_disolved; });
	std::vector<FluidGroup*> unstable2(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable2)
		fluidGroup->mergeStep();
	std::erase_if(m_unstableFluidGroups, [](FluidGroup* fluidGroup){ return fluidGroup->m_merged; });
	// Apply fluid split.
	std::vector<FluidGroup*> unstable3(m_unstableFluidGroups.begin(), m_unstableFluidGroups.end());
	for(FluidGroup* fluidGroup : unstable3)
		fluidGroup->splitStep();
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	std::unordered_set<FluidGroup*> toErase;
	for(FluidGroup& fluidGroup : m_fluidGroups)
	{
		if(fluidGroup.m_excessVolume <= 0 && fluidGroup.m_drainQueue.m_set.size() == 0)
		{
			fluidGroup.m_destroy = true;
			assert(!fluidGroup.m_disolved);
		}
		if(fluidGroup.m_destroy || fluidGroup.m_merged || fluidGroup.m_disolved || fluidGroup.m_stable)
			m_unstableFluidGroups.erase(&fluidGroup);
		else if(!fluidGroup.m_stable) // This seems avoidable.
			m_unstableFluidGroups.insert(&fluidGroup);
		if(fluidGroup.m_destroy || fluidGroup.m_merged)
		{
			toErase.insert(&fluidGroup);
			if(fluidGroup.m_destroy)
				assert(fluidGroup.m_drainQueue.m_set.empty());
		}
	}
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate(toErase);
	m_fluidGroups.remove_if([&](FluidGroup& fluidGroup){ return toErase.contains(&fluidGroup); });
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	for(const FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		[[maybe_unused]] bool found = false;
		for(FluidGroup& fg : m_fluidGroups)
			if(&fg == fluidGroup)
			{
				found = true;
				continue;
			}
		assert(found);
	}
	// Apply cave in.
	if(!m_caveInData.empty())
		stepCaveInWrite();
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// If there is any unstable groups expire route caches.
	// TODO: Be more selective?
	for(FluidGroup& fluidGroup : m_fluidGroups)
		fluidGroup.validate();
	// Clean up old vision cuboids.
	if(m_visionCuboidsActive)
		std::erase_if(m_visionCuboids, [](VisionCuboid& visionCuboid){ return visionCuboid.m_destroy; });
	// Apply vision.
	m_hasActors.processVisionWriteStep();
	// Apply temperature deltas.
	m_hasTemperature.applyDeltas();
	// Apply rain.
	m_hasRain.writeStep();
	// Apply fluid Sources.
	m_fluidSources.step();
}
Block& Area::getBlock(uint32_t x, uint32_t y, uint32_t z)
{
	size_t index = x + (y * m_sizeX) + (z * m_sizeY * m_sizeX); 
	return m_blocks[index];
}
Block& Area::getGroundLevel(uint32_t x, uint32_t y)
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
	uint32_t x = (m_sizeX - 1) / 2;
	uint32_t y = (m_sizeY - 1) / 2;
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
FluidGroup* Area::createFluidGroup(const FluidType& fluidType, std::unordered_set<Block*>& blocks, bool checkMerge)
{
	m_fluidGroups.emplace_back(fluidType, blocks, *this, checkMerge);
	m_unstableFluidGroups.insert(&m_fluidGroups.back());
	//TODO:  If new group is outside register it with areaHasTemperature.
	return &m_fluidGroups.back();
}
void Area::visionCuboidsActivate()
{
	m_visionCuboidsActive = true;
	VisionCuboid::setup(*this);
}
void Area::setDateTime(DateTime now)
{
	//TODO: daylight.
	m_hasTemperature.setAmbientSurfaceTemperatureFor(now);
	if(now.hour == 1)
	{
		for(Plant& plant : m_hasPlants.getAll())
			plant.setDayOfYear(now.day);
		m_hasFarmFields.setDayOfYear(now.day);
	}
}
Cuboid Area::getZLevel(uint32_t z)
{
	return Cuboid(getBlock(m_sizeX - 1, m_sizeY - 1, z), getBlock(0, 0, z));
}
void Area::validateAllFluidGroups()
{
	for(FluidGroup& fluidGroup : m_fluidGroups)
		if(!fluidGroup.m_merged && !fluidGroup.m_destroy)
			fluidGroup.validate();
}
std::string Area::toS()
{
	std::string output = std::to_string(m_fluidGroups.size()) + " fluid groups########";
	for(FluidGroup& fluidGroup : m_fluidGroups)
	{
		output += "type:" + fluidGroup.m_fluidType.name;
		output += "-total:" + std::to_string(fluidGroup.totalVolume());
		output += "-blocks:" + std::to_string(fluidGroup.m_drainQueue.m_set.size());
		output += "-status:";
		if(fluidGroup.m_merged)
			output += "-merged";
		if(fluidGroup.m_stable)
			output += "-stable";
		if(fluidGroup.m_disolved)
		{
			output += "-disolved";
			for(FluidGroup& fg : m_fluidGroups)
				if(fg.m_disolvedInThisGroup.contains(&fluidGroup.m_fluidType) && fg.m_disolvedInThisGroup.at(&fluidGroup.m_fluidType) == &fluidGroup)
					output += " in " + fg.m_fluidType.name;
		}
		if(fluidGroup.m_destroy)
			output += "-destroy";
		output += "###";
	}
	return output;
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