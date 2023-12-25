/*
* A game map.
*/

#include "area.h"
#include "deserilizationMemo.h"
#include "fire.h"
#include "plant.h"
#include "simulation.h"
#include <algorithm>
#include <iostream>
#include <numeric>

Area::Area(AreaId id, Simulation& s, uint32_t x, uint32_t y, uint32_t z) :
	m_blocks(x*y*z), m_id(id), m_simulation(s), m_sizeX(x), m_sizeY(y), m_sizeZ(z), m_areaHasTemperature(*this), m_hasActors(*this), m_hasFarmFields(*this), m_hasStockPiles(*this), m_hasItems(*this), m_hasRain(*this), m_visionCuboidsActive(false)
{
	// build m_blocks
	for(uint32_t x = 0; x < m_sizeX; ++x)
		for(uint32_t y = 0; y < m_sizeY; ++y)
			for(uint32_t z = 0; z < m_sizeZ; ++z)
				getBlock(x, y, z).setup(*this, x, y, z);
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
	m_simulation.m_pool.push_task([&](){ stepCaveInRead(); });
	// Calculate flow.
	for(FluidGroup* fluidGroup : m_unstableFluidGroups)
	{
		assert(!fluidGroup->m_destroy);
		m_simulation.m_pool.push_task([=](){ fluidGroup->readStep(); });
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
	m_areaHasTemperature.applyDeltas();
	// Apply rain.
	m_hasRain.writeStep();
}
Block& Area::getBlock(uint32_t x, uint32_t y, uint32_t z)
{
	size_t index = x + (y * m_sizeX) + (z * m_sizeY * m_sizeX); 
	return m_blocks[index];
}
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
	m_areaHasTemperature.setAmbientSurfaceTemperatureFor(now);
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
Json Area::toJson() const
{
	Json data;
	data["id"] = m_id;
	data["sizeX"] = m_sizeX;
	data["sizeY"] = m_sizeY;
	data["sizeZ"] = m_sizeZ;
	data["hasSleepingSpots"] = m_hasSleepingSpots.toJson();
	return data;
}
void Area::loadProjects(const Json& data, DeserilizationMemo& deserilizationMemo)
{
	m_hasDigDesignations.load(data["hasDigDesignations"], deserilizationMemo);
	m_hasConstructionDesignations.load(data["hasConstructionDesignations"], deserilizationMemo);
	m_hasStockPiles.load(data["hasStockpiles"], deserilizationMemo);
	m_hasCraftingLocationsAndJobs.load(data["hasCraftingLocationsAndJobs"], deserilizationMemo);
	m_targetedHauling.load(data["targetedHauling"], deserilizationMemo);
}
void Area::loadPlantFromJson(Json& data)
{
	Block& location = m_simulation.getBlockForJsonQuery(data["location"]);
	const PlantSpecies& species = PlantSpecies::byName(data["species"].get<std::string>());
	Percent percentGrowth = data["percentGrowth"].get<Percent>();
	Volume volumeFluidRequested = data["volumeFluidRequested"].get<Volume>();
	Step needsFluidEventStart = data["needsFluidEventStart"].get<Step>();
	bool temperatureIsUnsafe = data["temperatureIsUnsafe"].get<bool>();
	Step unsafeTemperatureEventStart = data["unsafeTemperatureEventStart"].get<Step>();
	uint32_t harvestableQuantity = data["harvestableQuantity"].get<uint32_t>();
	Percent percentFoliage = data["perecntFoliage"].get<Percent>();
	m_hasPlants.emplace(location, species, percentGrowth, volumeFluidRequested, needsFluidEventStart, temperatureIsUnsafe, unsafeTemperatureEventStart, harvestableQuantity, percentFoliage);
}
void Area::loadFireFromJson(Json& data)
{
	Block& location = m_simulation.getBlockForJsonQuery(data["location"]);
	const MaterialType& materialType = MaterialType::byName(data["materialType"].get<std::string>());
	FireStage stage = fireStageByName(data["stage"].get<std::string>());
	bool hasPeaked = data["hasPeaked"].get<bool>();
	Step stageStart = data["stageStart"].get<Percent>();
	m_fires.load(location, materialType, hasPeaked, stage, stageStart);
}
