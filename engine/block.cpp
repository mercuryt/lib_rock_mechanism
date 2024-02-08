/*
 * A block. Contains either a single type of material in 'solid' form or arbitrary objects with volume, generic solids and liquids.
 * Total volume is 100.
 */

#include "deserializationMemo.h"
#include "materialType.h"
#include "moveType.h"
#include "shape.h"
#include "hasShape.h"
#include "block.h"
#include "area.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <math.h>
#include <algorithm>
#include <cassert>

Block::Block() : m_solid(nullptr), m_constructed(false), m_totalFluidVolume(0), m_mist(nullptr), m_mistSource(nullptr),  m_mistInverseDistanceFromSource(0), m_visionCuboid(nullptr), m_fires(nullptr), m_exposedToSky(true), m_underground(false), m_outdoors(true), m_visible(true), m_hasShapes(*this), m_reservable(1), m_hasPlant(*this), m_hasBlockFeatures(*this), m_hasActors(*this), m_hasItems(*this), m_isPartOfStockPiles(*this), m_isPartOfFarmField(*this), m_blockHasTemperature(*this) {}
void Block::setup(Area& area, uint32_t ax, uint32_t ay, uint32_t az)
{
	m_x=ax;
	m_y=ay;
	m_z=az;
	m_area = &area;
	m_locationBucket = m_area->m_hasActors.m_locationBuckets.getBucketFor(*this);
	m_isEdge = (m_x == 0 || m_x == (m_area->m_sizeX - 1) ||  m_y == 0 || m_y == (m_area->m_sizeY - 1) || m_z == 0 || m_z == (m_area->m_sizeZ - 1) );
	// This is too slow to do on init.
	//uint32_t seed = (m_x * 1'000'000) + (m_y * 1'000) + m_z;
	//m_seed = m_area->m_simulation.m_random.deterministicScramble(seed);
}
void Block::recordAdjacent()
{
	static const int32_t offsetsList[6][3] = {{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}};
	for(uint32_t i = 0; i < 6; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = m_adjacents[i] = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			m_adjacentsVector.push_back(block);
	}
}
std::vector<Block*> Block::getAdjacentWithEdgeAdjacent() const
{
	std::vector<Block*> output;
	output.reserve(18);
	static const int32_t offsetsList[18][3] = {
		{-1,0,-1}, 
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,0,-1}, 

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {0,-1,0},

		{-1,0,1}, 
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,0,1}, 
	};
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
// TODO: cache.
std::vector<Block*> Block::getAdjacentWithEdgeAndCornerAdjacent() const
{
	std::vector<Block*> output;
	output.reserve(26);
	static const int32_t offsetsList[26][3] = {
		{-1,1,-1}, {-1,0,-1}, {-1,-1,-1},
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,1,-1}, {1,0,-1}, {1,-1,-1},

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {-1,1,0},

		{-1,1,1}, {-1,0,1}, {-1,-1,1},
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,1,1}, {1,0,1}, {1,-1,1}
	};
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getEdgeAdjacentOnly() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 

		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},

		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getEdgeAdjacentOnSameZLevelOnly() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getEdgeAdjacentOnlyOnNextZLevelDown() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getEdgeAndCornerAdjacentOnlyOnNextZLevelDown() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,-1}, {-1,0,-1}, {-1, 1, -1},
		{0,-1,-1}, {0,1,-1},
		{1,-1,-1}, {1,0,-1}, {1,1,-1}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getEdgeAdjacentOnlyOnNextZLevelUp() const
{
	std::vector<Block*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getEdgeAndCornerAdjacentOnly() const
{
	std::vector<Block*> output;
	output.reserve(20);
	static const int32_t offsetsList[20][3] = {
		{-1,-1,-1}, {-1,0,-1}, {0,-1,-1},
		{1,1,-1}, {0,1,-1},
		{1,-1,-1}, {1,0,-1}, {0,1,-1}, 

		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},

		{-1,-1,1}, {-1,0,1}, {0,-1,1},
		{1,1,1}, {0,1,1},
		{1,-1,1}, {1,0,1}, {0,-1,1}
	};
	for(uint32_t i = 0; i < 20; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<Block*> Block::getAdjacentOnSameZLevelOnly() const
{
	std::vector<Block*> output;
	output.reserve(4);
	static const int32_t offsetsList[4][3] = {
		{-1,0,0}, {1,0,0}, 
		{0,-1,0}, {0,1,0}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		Block* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
uint32_t Block::distance(Block& block) const
{
	uint32_t dx = abs((int)m_x - (int)block.m_x);
	uint32_t dy = abs((int)m_y - (int)block.m_y);
	uint32_t dz = abs((int)m_z - (int)block.m_z);
	return pow((pow(dx, 2) + pow(dy, 2) + pow(dz, 2)), 0.5);
}
uint32_t Block::taxiDistance(const Block& block) const
{
	return abs((int)m_x - (int)block.m_x) + abs((int)m_y - (int)block.m_y) + abs((int)m_z - (int)block.m_z);
}
bool Block::squareOfDistanceIsMoreThen(const Block& block, uint32_t distanceCubed) const
{
	uint32_t dx = abs((int32_t)block.m_x - (int32_t)m_x);
	uint32_t dy = abs((int32_t)block.m_y - (int32_t)m_y);
	uint32_t dz = abs((int32_t)block.m_z - (int32_t)m_z);
	return (dx * dx) + (dy * dy) + (dz * dz) > distanceCubed;
}
bool Block::isAdjacentToAny(std::unordered_set<Block*>& blocks) const
{
	for(Block* adjacent : m_adjacentsVector)
		if(blocks.contains(adjacent))
			return true;
	return false;
}
bool Block::isAdjacentTo(Block& block) const
{
	for(Block* adjacent : m_adjacentsVector)
		if(&block == adjacent)
			return true;
	return false;
}
bool Block::isAdjacentToIncludingCornersAndEdges(Block& block) const
{
	for(Block* adjacent : getAdjacentWithEdgeAndCornerAdjacent())
		if(&block == adjacent)
			return true;
	return false;
}
bool Block::isAdjacentTo(HasShape& hasShape) const
{
	return hasShape.getAdjacentBlocks().contains(const_cast<Block*>(this));
}
void Block::setNotSolid()
{
	if(m_solid == nullptr)
		return;
	m_solid = nullptr;
	m_constructed = false;
	for(Block* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_fluids)
			{
				pair.second->m_fillQueue.addBlock(this);
				pair.second->m_stable = false;
			}
	m_hasShapes.clearCache();
	if(m_area->m_visionCuboidsActive)
		VisionCuboid::BlockIsNeverOpaque(*this);
	if(m_adjacents[5] == nullptr || m_adjacents[5]->m_exposedToSky)
	{
		setExposedToSky(true);
		setBelowExposedToSky();
	}
	//TODO: Check if any adjacent are visible first?
	m_visible = true;
	setBelowVisible();
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid block.
	m_reservable.clearAll();
}
void Block::setExposedToSky(bool exposed)
{
	m_exposedToSky = exposed;
	m_hasPlant.updateGrowingStatus();
}
void Block::setBelowExposedToSky()
{
	Block* block = m_adjacents[0];
	while(block != nullptr && block->canSeeThroughFrom(*block->m_adjacents[5]) && !block->m_exposedToSky)
	{
		block->setExposedToSky(true);
		block = block->m_adjacents[0];
	}
}
void Block::setBelowVisible()
{
	Block* block = getBlockBelow();
	while(block && block->canSeeThroughFrom(*block->m_adjacents[5]) && !block->m_visible)
	{
		block->m_visible = true;
		block = block->m_adjacents[0];
	}
}
void Block::setBelowNotExposedToSky()
{
	Block* block = m_adjacents[0];
	while(block != nullptr && block->m_exposedToSky)
	{
		block->setExposedToSky(false);
		block = block->m_adjacents[0];
	}
}
void Block::setSolid(const MaterialType& materialType, bool constructed)
{
	assert(m_hasItems.empty());
	if(&materialType == m_solid)
		return;
	bool wasEmpty = m_solid == nullptr;
	m_solid = &materialType;
	m_constructed = constructed;
	// Displace fluids.
	m_totalFluidVolume = 0;
	for(auto& [fluidType, pair] : m_fluids)
	{
		pair.second->removeBlock(*this);
		pair.second->addFluid(pair.first);
		// If there is no where to put the fluid.
		if(pair.second->m_drainQueue.m_set.empty() && pair.second->m_fillQueue.m_set.empty())
		{
			// If fluid piston is enabled then find a place above to add to potential.
			if constexpr (Config::fluidPiston)
			{
				Block* above = m_adjacents[5];
				while(above != nullptr)
				{
					if(above->fluidCanEnterEver() && above->fluidCanEnterCurrently(*fluidType))
					{
						pair.second->m_fillQueue.addBlock(above);
						break;
					}
					above = above->m_adjacents[5];
				}
				// The only way that this 'piston' code should be triggered is if something falls, which means the top layer cannot be full.
				assert(above != nullptr);
			}
			else
			{
				// Otherwise destroy the group.
				m_area->m_fluidGroups.remove(*pair.second);
				m_area->m_unstableFluidGroups.erase(pair.second);
			}
		}
	}
	m_fluids.clear();
	// Remove from fluid fill queues.
	for(Block* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver())
			for(auto& [fluidType, pair] : adjacent->m_fluids)
				pair.second->m_fillQueue.removeBlock(this);
	m_visible = false;
	// Clear move cost caches for this and adjacent
	m_hasShapes.clearCache();
	// Opacity.
	if(m_area->m_visionCuboidsActive && !materialType.transparent)
		VisionCuboid::BlockIsSometimesOpaque(*this);
	// Set blocks below as not exposed to sky.
	setExposedToSky(false);
	setBelowNotExposedToSky();
	// Remove from stockpiles.
	m_area->m_hasStockPiles.removeBlockFromAllFactions(*this);
	m_area->m_hasCraftingLocationsAndJobs.maybeRemoveLocation(*this);
	if(wasEmpty)
		// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid block.
		m_reservable.clearAll();
}
uint32_t Block::getMass() const
{
	assert(isSolid());
	return m_solid->density * Config::maxBlockVolume;
}
bool Block::isSolid() const
{
	return m_solid != nullptr;
}
const MaterialType& Block::getSolidMaterial() const
{
	return *m_solid;
}
bool Block::canSeeIntoFromAlways(const Block& block) const
{
	if(isSolid() && !getSolidMaterial().transparent)
		return false;
	if(m_hasBlockFeatures.contains(BlockFeatureType::door))
		return false;
	// looking up.
	if(m_z > block.m_z)
	{
		const BlockFeature* floor = m_hasBlockFeatures.at(BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		if(m_hasBlockFeatures.contains(BlockFeatureType::hatch))
			return false;
	}
	// looking down.
	if(m_z < block.m_z)
	{
		const BlockFeature* floor = block.m_hasBlockFeatures.at(BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = block.m_hasBlockFeatures.at(BlockFeatureType::hatch);
		if(hatch != nullptr && !hatch->materialType->transparent)
			return false;
	}
	return true;
}
void Block::moveContentsTo(Block& block)
{
	if(isSolid())
	{
		const MaterialType& materialType = getSolidMaterial();
		setNotSolid();
		block.setSolid(materialType);
	}
	//TODO: other stuff falls?
}
Block* Block::offset(int32_t ax, int32_t ay, int32_t az) const
{
	ax += m_x;
	ay += m_y;
	az += m_z;
	if(ax < 0 || (uint32_t)ax >= m_area->m_sizeX || ay < 0 || (uint32_t)ay >= m_area->m_sizeY || az < 0 || (uint32_t)az >= m_area->m_sizeZ)
		return nullptr;
	return &m_area->getBlock(ax, ay, az);
}
std::array<int32_t, 3> Block::relativeOffsetTo(const Block& block) const
{
	return {(int)block.m_x - (int)m_x, (int)block.m_y - (int)m_y, (int)block.m_z - (int)m_z};
}
bool Block::canSeeThrough() const
{
	if(isSolid() && !getSolidMaterial().transparent)
		return false;
	const BlockFeature* door = m_hasBlockFeatures.at(BlockFeatureType::door);
	if(door != nullptr && !door->materialType->transparent && door->closed)
		return false;
	return true;
}
bool Block::canSeeThroughFrom(const Block& block) const
{
	if(!canSeeThrough())
		return false;
	// On the same level.
	if(m_z == block.m_z)
		return true;
	// looking up.
	if(m_z > block.m_z)
	{
		const BlockFeature* floor = m_hasBlockFeatures.at(BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = m_hasBlockFeatures.at(BlockFeatureType::hatch);
		if(hatch != nullptr && !hatch->materialType->transparent && hatch->closed)
			return false;
	}
	// looking down.
	if(m_z < block.m_z)
	{
		const BlockFeature* floor = block.m_hasBlockFeatures.at(BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = block.m_hasBlockFeatures.at(BlockFeatureType::hatch);
		if(hatch != nullptr && !hatch->materialType->transparent && hatch->closed)
			return false;
	}
	return true;
}
uint8_t Block::facingToSetWhenEnteringFrom(const Block& block) const
{
	if(block.m_x > m_x)
		return 3;
	if(block.m_x < m_x)
		return 1;
	if(block.m_y < m_y)
		return 2;
	return 0;
}
bool Block::isSupport() const
{
	return isSolid() || m_hasBlockFeatures.isSupport();
}
bool Block::hasLineOfSightTo(Block& block) const
{
	return VisionRequest::hasLineOfSightBasic(*this, block);
}
void Block::spawnMist(const FluidType& fluidType, uint32_t maxMistSpread)
{
	if(m_mist != nullptr && (m_mist == &fluidType || m_mist->density > fluidType.density))
		return;
	m_mist = &fluidType;
	m_mistInverseDistanceFromSource = maxMistSpread != 0 ? maxMistSpread : fluidType.maxMistSpread;
	MistDisperseEvent::emplace(fluidType.mistDuration, fluidType, *this);
}
//TODO: This code puts the fluid into an adjacent group of the correct type if it can find one, it does not add the block or merge groups, leaving these tasks to fluidGroup readStep. Is this ok?
void Block::addFluid(uint32_t volume, const FluidType& fluidType)
{
	// If a suitable fluid group exists already then just add to it's excessVolume.
	if(m_fluids.contains(&fluidType))
	{
		assert(m_fluids.at(&fluidType).second->m_fluidType == fluidType);
		m_fluids.at(&fluidType).second->addFluid(volume);
		return;
	}
	m_fluids.emplace(&fluidType, std::make_pair(volume, nullptr));
	m_totalFluidVolume += volume;
	// Find fluid group.
	FluidGroup* fluidGroup = nullptr;
	for(Block* adjacent : m_adjacentsVector)
		if(adjacent->fluidCanEnterEver() && adjacent->m_fluids.contains(&fluidType))
		{
			assert(adjacent->m_fluids.at(&fluidType).second->m_fluidType == fluidType);
			fluidGroup = adjacent->m_fluids.at(&fluidType).second;
			fluidGroup->addBlock(*this);
			continue;
		}
	// Create fluid group.
	if(fluidGroup == nullptr)
	{
		std::unordered_set<Block*> blocks({this});
		fluidGroup = m_area->createFluidGroup(fluidType, blocks);
	}
	// Shift less dense fluids to excessVolume.
	if(m_totalFluidVolume > Config::maxBlockVolume)
		resolveFluidOverfull();
}
void Block::removeFluid(uint32_t volume, const FluidType& fluidType)
{
	assert(volume <= volumeOfFluidTypeContains(fluidType));
	m_fluids.at(&fluidType).second->removeFluid(volume);
}
bool Block::fluidCanEnterCurrently(const FluidType& fluidType) const
{
	if(m_totalFluidVolume < Config::maxBlockVolume)
		return true;
	for(auto& pair : m_fluids)
		if(pair.first->density < fluidType.density)
			return true;
	return false;
}
bool Block::fluidCanEnterEver() const
{
	return !isSolid();
}
bool Block::isAdjacentToFluidGroup(const FluidGroup* fluidGroup) const
{
	for(Block* block : m_adjacentsVector)
		if(block->m_fluids.contains(&fluidGroup->m_fluidType) && block->m_fluids.at(&fluidGroup->m_fluidType).second == fluidGroup)
			return true;
	return false;
}
uint32_t Block::volumeOfFluidTypeCanEnter(const FluidType& fluidType) const
{
	uint32_t output = Config::maxBlockVolume;
	for(auto& pair : m_fluids)
		if(pair.first->density >= fluidType.density)
			output -= pair.second.first;
	return output;
}
uint32_t Block::volumeOfFluidTypeContains(const FluidType& fluidType) const
{
	auto found = m_fluids.find(&fluidType);
	if(found == m_fluids.end())
		return 0;
	return found->second.first;
}
const FluidType& Block::getFluidTypeWithMostVolume() const
{
	assert(!m_fluids.empty());
	uint32_t volume = 0;
	FluidType* output = nullptr;
	for(auto [fluidType, pair] : m_fluids)
		if(volume < pair.first)
			output = const_cast<FluidType*>(fluidType);
	assert(output != nullptr);
	return *output;
}
FluidGroup* Block::getFluidGroup(const FluidType& fluidType) const
{
	auto found = m_fluids.find(&fluidType);
	if(found == m_fluids.end())
		return nullptr;
	assert(found->second.second->m_fluidType == fluidType);
	return found->second.second;
}
void Block::resolveFluidOverfull()
{
	std::vector<const FluidType*> toErase;
	// Fluid types are sorted by density.
	for(auto& [fluidType, pair] : m_fluids)
	{
		assert(fluidType == &pair.second->m_fluidType);
		// Displace lower density fluids.
		uint32_t displaced = std::min(pair.first, m_totalFluidVolume - Config::maxBlockVolume);
		m_totalFluidVolume -= displaced;
		pair.first -= displaced;
		pair.second->addFluid(displaced);
		if(pair.first == 0)
			toErase.push_back(fluidType);
		if(pair.first < Config::maxBlockVolume)
			pair.second->m_fillQueue.addBlock(this);
		if(m_totalFluidVolume == Config::maxBlockVolume)
			break;
	}
	for(const FluidType* fluidType : toErase)
	{
		// If the last block of a fluidGroup is displaced disolve it in the lowest density liquid which is more dense then it.
		FluidGroup* fluidGroup = m_fluids.at(fluidType).second;
		assert(fluidGroup->m_fluidType == *fluidType);
		fluidGroup->removeBlock(*this);
		if(fluidGroup->m_drainQueue.m_set.empty())
		{
			for(auto& [otherFluidType, pair] : m_fluids)
				if(otherFluidType->density > fluidType->density)
				{
					//TODO: find.
					if(pair.second->m_disolvedInThisGroup.contains(fluidType))
					{
						pair.second->m_disolvedInThisGroup.at(fluidType)->m_excessVolume += fluidGroup->m_excessVolume;
						fluidGroup->m_destroy = true;
					}
					else
					{
						pair.second->m_disolvedInThisGroup[fluidType] = fluidGroup;
						fluidGroup->m_disolved = true;
					}
					break;
				}
			assert(fluidGroup->m_disolved || fluidGroup->m_destroy);
		}
		m_fluids.erase(fluidType);
	}
}
bool Block::operator==(const Block& block) const { return &block == this; };
//TODO: Replace with cuboid.
std::vector<Block*> Block::selectBetweenCorners(Block* otherBlock) const
{
	assert(otherBlock->m_x < m_area->m_sizeX);
	assert(otherBlock->m_y < m_area->m_sizeY);
	assert(otherBlock->m_z < m_area->m_sizeZ);
	std::vector<Block*> output;
	uint32_t minX = std::min(m_x, otherBlock->m_x);
	uint32_t maxX = std::max(m_x, otherBlock->m_x);
	uint32_t minY = std::min(m_y, otherBlock->m_y);
	uint32_t maxY = std::max(m_y, otherBlock->m_y);
	uint32_t minZ = std::min(m_z, otherBlock->m_z);
	uint32_t maxZ = std::max(m_z, otherBlock->m_z);
	for(uint32_t x = minX; x <= maxX; ++x)
		for(uint32_t y = minY; y <= maxY; ++y)
			for(uint32_t z = minZ; z <= maxZ; ++z)
			{
				output.push_back(&m_area->getBlock(x, y, z));
				assert(output.back() != nullptr);
			}
	return output;
}
std::unordered_set<Block*> Block::collectAdjacentsInRange(uint32_t range)
{
	auto condition = [&](Block& b){ return b.taxiDistance(*this) <= range; };
	return collectAdjacentsWithCondition(condition);
}
std::vector<Block*> Block::collectAdjacentsInRangeVector(uint32_t range)
{
	auto result = collectAdjacentsInRange(range);
	std::vector<Block*> output(result.begin(), result.end());
	return output;
}
void Block::loadFromJson(Json data, DeserializationMemo& deserializationMemo)
{
	m_exposedToSky = data["exposedToSky"].get<bool>();
	m_underground = data["underground"].get<bool>();
	m_outdoors = data["outdoors"].get<bool>();
	m_visible = data["visible"].get<bool>();
	if(data.contains("solid"))
		m_solid = &MaterialType::byName(data["solid"].get<std::string>());
	if(data.contains("blockFeatures"))
		for(Json blockFeatureData : data["blockFeatures"])
		{
			const BlockFeatureType& blockFeatureType = BlockFeatureType::byName(blockFeatureData["blockFeatureType"].get<std::string>());
			const MaterialType& materialType = MaterialType::byName(blockFeatureData["materialType"].get<std::string>());
			if(blockFeatureData.contains("hewn"))
			{
				m_solid = &materialType;
				m_hasBlockFeatures.hew(blockFeatureType);
			}
			else
				m_hasBlockFeatures.construct(blockFeatureType, materialType);
		}
	if(data.contains("fluids"))
		for(const Json& pair : data["fluids"])
			addFluid(pair[1].get<Volume>(), *pair[0].get<const FluidType*>());
	if(data.contains("hasDesignations"))
		m_hasDesignations.load(data["hasDesignations"], deserializationMemo);
	m_visible = data.contains("visible");
}
Json Block::toJson() const 
{
	Json data{{"exposedToSky", m_exposedToSky}, {"underground", m_underground}, {"isEdge", m_isEdge}, {"outdoors", m_outdoors}, {"visible", m_visible}, {"x", m_x}, {"y", m_y}, {"z", m_z}, {"area", m_area->m_id}};
	if(m_solid != nullptr)
		data["solid"] = m_solid->name;
	if(!m_hasBlockFeatures.empty())
	{
		data["blockFeatures"] = Json::array();
		for(const BlockFeature& blockFeature : m_hasBlockFeatures.get())
		{
			Json blockFeatureData{{"materialType", blockFeature.materialType}, {"blockFeatureType", blockFeature.blockFeatureType}};
			if(blockFeature.hewn)
				blockFeatureData["hewn"] = true;
			data["blockFeatures"].push_back(blockFeatureData);
		}
	}
	if(m_totalFluidVolume != 0)
	{
		data["fluids"] = Json::array();
		for(auto& [fluidType, pair] : m_fluids)
			data["fluids"].push_back({fluidType, pair.first});
	}
	if(!m_hasDesignations.empty())
		data["hasDesignations"] = m_hasDesignations.toJson();
	return data;
}
Json Block::positionToJson() const { return {{"x", m_x}, {"y", m_y}, {"z", m_z}, {"area", m_area->m_id}}; }
