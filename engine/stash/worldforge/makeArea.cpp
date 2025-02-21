#include "../simulation/simulation.h"
#include "../nthAdjacentOffsets.h"
#include "worldLocation.h"
#include "world.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>

class Area;
struct AdjacentHeight
{
	size_t x;
	size_t y;
	size_t z;
};

Meters heightInBlocksWith(size_t averageHeightBlocks, Meters averageHeightMeters, Meters height)
{
	Meters difference = height - averageHeightMeters;
	size_t blocksDifference = difference * Config::blocksPerMeter;
	return averageHeightBlocks + blocksDifference;
}
Area& World::makeArea(WorldLocation& location)
{
	Area& area = m_simulation.createArea(m_config.areaSizeX, m_config.areaSizeY, m_config.areaSizeZ);
	assignAreaHeights(location, area);
	assignAreaBiome(location, area);
	area.m_worldLocation = &location;
	location.area = area.m_id;
	return area;
}
void World::assignAreaHeights(WorldLocation& location, Area& area)
{
	Meters averageLandHeightAboveSeaLevel = 0;
	for(WorldLocation* adjacent : location.adjacent)
		averageLandHeightAboveSeaLevel += adjacent->landHeightAboveSeaLevel;
	// averageLandHeightAboveSeaLevel in meters is the same as m_config.landHeightZ in blocks.
	// Z height of adjacent locations is scaled from meters to match.
	averageLandHeightAboveSeaLevel /= location.adjacent.size();
	std::vector<AdjacentHeight> adjacentHeights;
	size_t x, y, z;
	// West.
	if(location.west)
	{
		x = 0;
		y = (area.m_sizeY - 1) / 2;
		z = heightInBlocksWith(m_config.averageLandHeightBlocks, averageLandHeightAboveSeaLevel, location.west->landHeightAboveSeaLevel);
		adjacentHeights.emplace_back(x, y, z);
	}
	// East.
	if(location.east)
	{
		x = area.m_sizeX - 1;
		y = (area.m_sizeY - 1) / 2;
		z = heightInBlocksWith(m_config.averageLandHeightBlocks, averageLandHeightAboveSeaLevel, location.east->landHeightAboveSeaLevel);
		adjacentHeights.emplace_back(x, y, z);
	}
	// North.
	if(!location.north.empty())
	{
		size_t spacing = area.m_sizeX / (location.north.size() - 1);
		x = 0;
		y = 0;
		for(WorldLocation* adjacent : location.north)
		{
			z = heightInBlocksWith(m_config.averageLandHeightBlocks, averageLandHeightAboveSeaLevel, adjacent->landHeightAboveSeaLevel);
			adjacentHeights.emplace_back(x, y, z);
			x += spacing;
		}
	}
	// North.
	if(!location.south.empty())
	{
		size_t spacing = area.m_sizeX / (location.south.size() - 1);
		x = 0;
		y = area.m_sizeY - 1;
		for(WorldLocation* adjacent : location.south)
		{
			z = heightInBlocksWith(m_config.averageLandHeightBlocks, averageLandHeightAboveSeaLevel, adjacent->landHeightAboveSeaLevel);
			adjacentHeights.emplace_back(x, y, z);
			x += spacing;
		}
	}
	// Assign heights.
	uint32_t maxDistance = area.getBlock(0,0,0).distance(area.getBlock(area.m_sizeX - 1, area.m_sizeY - 1, 0));
	for(size_t x = 0; x < area.m_sizeX; ++x)
		for(size_t y = 0; y < area.m_sizeY; ++y)
		{
			// For each block get the sum of values from adjacentHeights and scale by distance.
			BlockIndex& block = area.getBlock(x, y, 0);
			uint32_t totalInfluence = 0;
			std::vector<size_t> heightTimesInfluence;
			for(AdjacentHeight& height : adjacentHeights)
			{
				BlockIndex& adjacentBlock = area.getBlock(height.x, height.y, 0);
				uint32_t distance = block.distance(adjacentBlock);
				uint32_t influence = util::scaleByInverseFraction(1000, distance, maxDistance);
				totalInfluence += influence;
				heightTimesInfluence.push_back(height.z * influence);
			}
			size_t finalHeight = averageLandHeightAboveSeaLevel;
			for(size_t height : heightTimesInfluence)
				finalHeight += height / totalInfluence;
			// TODO: Geology.
			size_t layersOfDirt = 2;
			for(size_t z = 0; z < finalHeight - layersOfDirt; ++z)
				area.getBlock(x, y, z).setSolid(MaterialType::byName("shist"));
			for(size_t z = finalHeight - layersOfDirt; z < finalHeight; ++z)
				area.getBlock(x, y, z).setSolid(MaterialType::byName("dirt"));
		}
}
void World::assignAreaBiome(WorldLocation& location, Area& area)
{
	uint32_t totalInfluence = 0;
	for(auto& pair : location.biomeInfluence)
		totalInfluence += pair.second;
	for(size_t x = 0; x < area.m_sizeX; ++x)
		for(size_t y = 0; y < area.m_sizeY; ++y)
		{
			uint32_t generated = m_random.getInRange(0u, totalInfluence);
			for(auto& [biome, influence] : location.biomeInfluence)
				if(influence > generated)
				{
					biome->createPlantsAndRocks(area.getBlock(x, y, 0), area.m_simulation, area.m_simulation.m_random);
					break;
				}
		}
	location.primaryBiome->createAnimals(area.m_simulation, location, area, area.m_simulation.m_random);
}
struct Candidate
{
	BlockIndex* block;
	Candidate* previous;
	bool operator<(const Candidate& other) { return block->m_z < other.block->m_z; }
};
std::vector<BlockIndex*> getPathForRiver(BlockIndex& start, BlockIndex& end)
{
		std::list<Candidate> candidates{{&start, nullptr}};
		std::unordered_set<const BlockIndex*> closedList{&start};
		std::priority_queue<Candidate*> openList;
		openList.push(&candidates.front());
		while(!openList.empty())
		{
			Candidate* candidate = openList.top();
			openList.pop();
			for(BlockIndex* adjacent : candidate->block->m_adjacents)
				if(adjacent != BLOCK_INDEX_MAX)
				{
					if(adjacent == &end)
					{
						// Found.
						std::vector<BlockIndex*> path{adjacent, candidate->block};
						while(candidate->previous)
						{
							candidate = candidate->previous;
							path.push_back(candidate->block->getBlockBelow() ? candidate->block->getBlockBelow() : candidate->block);
						}
						std::ranges::reverse(path);
						for(size_t i = 0; i < path.size(); ++i)
						{
							// Prevent flowing uphill.
							if(i && path[i - 1]->m_z < path[i]->m_z)
								path[i] = &start.m_area->getBlock(path[i]->m_x, path[i]->m_y, path[i - 1]->m_z);
						}
						return path;
					}
					if(closedList.contains(adjacent))
						continue;
					closedList.insert(adjacent);
					if(adjacent->fluidCanEnterEver())
						openList.push(&candidates.emplace_back(adjacent, candidate));
				}
		}
		// No path found.
		std::unreachable();
}
void World::makeAreaRiversAndLakes(WorldLocation& location, Area& area)
{
	static const FluidType& water = FluidType::byName("water");
	for(const River* river : location.rivers)
	{
		WorldLocation* upStream = const_cast<River*>(river)->getUpStream(location);
		WorldLocation* downStream = const_cast<River*>(river)->getDownStream(location);
		BlockIndex& start = upStream ? area.getBlockForAdjacentLocation(*upStream) : area.getMiddleAtGroundLevel();
		BlockIndex& end = downStream ? area.getBlockForAdjacentLocation(*downStream) : area.getMiddleAtGroundLevel();
		assert(start != end);
		auto path = getPathForRiver(start, end);
		std::unordered_set<BlockIndex*> blocksForRiver;
		for(BlockIndex* block : path)
		{
			// expand
			for(BlockIndex* adjacent : getNthAdjacentBlocks(*block, river->m_rate))
				if(adjacent->m_z <= block->m_z)
					blocksForRiver.insert(adjacent);
		}
		for(BlockIndex* block : blocksForRiver)
		{
			// Set not solid.
			BlockIndex* b = block;
			while(b->isSolid())
			{
				b->setNotSolid();
				b = b->getBlockAbove();
			}
			// Fill with water.
			int difference = Config::maxBlockVolume - block->m_totalFluidVolume;
			if(difference > 0)
				block->addFluid(difference, water);
		}
		area.m_fluidSources.create(start, water, Config::maxBlockVolume);
	}
	// TODO: lakes that extend over more then one location.
	if(location.lake)
	{
		std::unordered_set<BlockIndex*> blocks;
		for(uint8_t i = 0; i < location.lake->size; ++i)
		{
			uint32_t radius = location.lake->size * Config::lakeRadiusModifier;
			uint32_t depth = location.lake->size * Config::lakeDepthModifier;
			uint32_t centerX = (area.m_sizeX - 1) / 2;
			uint32_t centerY = (area.m_sizeY - 1) / 2;
			uint32_t x = m_random.getInRange(centerX - radius, centerX + radius);
			uint32_t y = m_random.getInRange(centerY - radius, centerY + radius);
			for(BlockIndex* block : getNthAdjacentBlocks(area.getGroundLevel(x, y), radius))
				if(block->m_z <= depth)
					blocks.insert(block);

		}
		for(BlockIndex* block : blocks)
		{
			// Set not solid.
			BlockIndex* b = block;
			while(b->isSolid())
			{
				b->setNotSolid();
				b = b->getBlockAbove();
			}
			// Fill with water.
			int difference = Config::maxBlockVolume - block->m_totalFluidVolume;
			if(difference > 0)
				block->addFluid(difference, water);
		}
	}
}
