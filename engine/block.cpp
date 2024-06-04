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

BlockIndex::BlockIndex() : m_reservable(1), m_hasFluids(*this), m_hasShapes(*this), m_hasActors(*this), m_hasItems(*this), m_isPartOfStockPiles(*this), m_isPartOfFarmField(*this), m_hasBlockFeatures(*this), m_hasPlant(*this), m_blockHasTemperature(*this) {}
void BlockIndex::setup(Area& area, DistanceInBlocks ax, DistanceInBlocks ay, DistanceInBlocks az)
{
	assert(this >= &area.getBlocks().front());
	m_x=ax;
	m_y=ay;
	m_z=az;
	m_area = &area;
	assert(getIndex() < area.getBlocks().size());
	m_locationBucket = &m_area->m_hasActors.m_locationBuckets.getBucketFor(*this);
	m_isEdge = (m_x == 0 || m_x == (m_area->m_sizeX - 1) ||  m_y == 0 || m_y == (m_area->m_sizeY - 1) || m_z == 0 || m_z == (m_area->m_sizeZ - 1) );
}
void BlockIndex::recordAdjacent()
{
	static const int32_t offsetsList[6][3] = {{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}};
	for(uint33_t i = 0; i < 6; i++)
	{
		auto& offsets = offsetsList[i];
		m_adjacents[i] = offset(offsets[0],offsets[1],offsets[2]);
	}
}
void BlockIndex::setDesignation(Faction& faction, BlockDesignation designation)
{
	m_area->m_blockDesignations.at(faction).set(getIndex(), designation);
}
void BlockIndex::unsetDesignation(Faction& faction, BlockDesignation designation)
{
	m_area->m_blockDesignations.at(faction).unset(getIndex(), designation);
}
void BlockIndex::maybeUnsetDesignation(Faction& faction, BlockDesignation designation)
{
	m_area->m_blockDesignations.at(faction).maybeUnset(getIndex(), designation);
}
bool BlockIndex::hasDesignation(Faction& faction, BlockDesignation designation) const
{
	return m_area->m_blockDesignations.at(faction).check(getIndex(), designation);
}
BlockIndex BlockIndex::getIndex() const { return m_area->getBlockIndex(*this); }
std::vector<BlockIndex*> BlockIndex::getAdjacentWithEdgeAdjacent() const
{
	std::vector<BlockIndex*> output;
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
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
// TODO: cache.
std::vector<BlockIndex*> BlockIndex::getAdjacentWithEdgeAndCornerAdjacent() const
{
	std::vector<BlockIndex*> output;
	output.reserve(26);
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsListAllAdjacent[i];
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getEdgeAdjacentOnly() const
{
	std::vector<BlockIndex*> output;
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
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getEdgeAdjacentOnSameZLevelOnly() const
{
	std::vector<BlockIndex*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getEdgeAdjacentOnlyOnNextZLevelDown() const
{
	std::vector<BlockIndex*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getEdgeAndCornerAdjacentOnlyOnNextZLevelDown() const
{
	std::vector<BlockIndex*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,-1}, {-1,0,-1}, {-1, 1, -1},
		{0,-1,-1}, {0,1,-1},
		{1,-1,-1}, {1,0,-1}, {1,1,-1}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getEdgeAdjacentOnlyOnNextZLevelUp() const
{
	std::vector<BlockIndex*> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getEdgeAndCornerAdjacentOnly() const
{
	std::vector<BlockIndex*> output;
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
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex*> BlockIndex::getAdjacentOnSameZLevelOnly() const
{
	std::vector<BlockIndex*> output;
	output.reserve(4);
	static const int32_t offsetsList[4][3] = {
		{-1,0,0}, {1,0,0}, 
		{0,-1,0}, {0,1,0}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex* block = offset(offsets[0],offsets[1],offsets[2]);
		if(block)
			output.push_back(block);
	}
	return output;
}
DistanceInBlocks BlockIndex::distance(BlockIndex& block) const
{
	DistanceInBlocks dx = abs((int)m_x - (int)block.m_x);
	DistanceInBlocks dy = abs((int)m_y - (int)block.m_y);
	DistanceInBlocks dz = abs((int)m_z - (int)block.m_z);
	return pow((pow(dx, 2) + pow(dy, 2) + pow(dz, 2)), 0.5);
}
DistanceInBlocks BlockIndex::taxiDistance(const BlockIndex& block) const
{
	return abs((int)m_x - (int)block.m_x) + abs((int)m_y - (int)block.m_y) + abs((int)m_z - (int)block.m_z);
}
bool BlockIndex::squareOfDistanceIsMoreThen(const BlockIndex& block, DistanceInBlocks distanceCubed) const
{
	DistanceInBlocks dx = abs((int32_t)block.m_x - (int32_t)m_x);
	DistanceInBlocks dy = abs((int32_t)block.m_y - (int32_t)m_y);
	DistanceInBlocks dz = abs((int32_t)block.m_z - (int32_t)m_z);
	return (dx * dx) + (dy * dy) + (dz * dz) > distanceCubed;
}
bool BlockIndex::isAdjacentToAny(std::unordered_set<BlockIndex*>& blocks) const
{
	for(BlockIndex* adjacent : m_adjacents)
		if(adjacent && blocks.contains(adjacent))
			return true;
	return false;
}
bool BlockIndex::isAdjacentTo(BlockIndex& block) const
{
	for(BlockIndex* adjacent : m_adjacents)
		if(adjacent && &block == adjacent)
			return true;
	return false;
}
bool BlockIndex::isAdjacentToIncludingCornersAndEdges(BlockIndex& block) const
{
	for(BlockIndex* adjacent : getAdjacentWithEdgeAndCornerAdjacent())
		if(&block == adjacent)
			return true;
	return false;
}
bool BlockIndex::isAdjacentTo(HasShape& hasShape) const
{
	return hasShape.getAdjacentBlocks().contains(const_cast<BlockIndex*>(this));
}
void BlockIndex::setNotSolid()
{
	if(m_solid == nullptr)
		return;
	m_solid = nullptr;
	m_constructed = false;
	m_hasFluids.onBlockSetNotSolid();
	m_hasShapes.clearCache();
	if constexpr(Config::visionCuboidsActive)
		m_area->m_hasActors.m_visionCuboids.blockIsNeverOpaque(*this);
	m_area->m_hasActors.m_opacityFacade.update(*this);
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
void BlockIndex::setExposedToSky(bool exposed)
{
	m_exposedToSky = exposed;
	m_hasPlant.updateGrowingStatus();
}
void BlockIndex::setBelowExposedToSky()
{
	BlockIndex* block = m_adjacents[0];
	while(block != nullptr && block->canSeeThroughFrom(*block->m_adjacents[5]) && !block->m_exposedToSky)
	{
		block->setExposedToSky(true);
		block = block->m_adjacents[0];
	}
}
void BlockIndex::setBelowVisible()
{
	BlockIndex* block = getBlockBelow();
	while(block && block->canSeeThroughFrom(*block->m_adjacents[5]) && !block->m_visible)
	{
		block->m_visible = true;
		block = block->m_adjacents[0];
	}
}
void BlockIndex::setBelowNotExposedToSky()
{
	BlockIndex* block = m_adjacents[0];
	while(block != nullptr && block->m_exposedToSky)
	{
		block->setExposedToSky(false);
		block = block->m_adjacents[0];
	}
}
void BlockIndex::setSolid(const MaterialType& materialType, bool constructed)
{
	assert(m_hasItems.empty());
	if(m_hasPlant.exists())
	{
		assert(!m_hasPlant.get().m_plantSpecies.isTree);
		m_hasPlant.get().die();
	}
	if(&materialType == m_solid)
		return;
	bool wasEmpty = m_solid == nullptr;
	m_solid = &materialType;
	m_constructed = constructed;
	m_hasFluids.onBlockSetSolid();
	m_visible = false;
	// Clear move cost caches for this and adjacent
	m_hasShapes.clearCache();
	// Opacity.
	if constexpr(Config::visionCuboidsActive)
	       if(!materialType.transparent && wasEmpty)
		       m_area->m_hasActors.m_visionCuboids.blockIsSometimesOpaque(*this);
	m_area->m_hasActors.m_opacityFacade.update(*this);
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
Mass BlockIndex::getMass() const
{
	assert(isSolid());
	return m_solid->density * Config::maxBlockVolume;
}
bool BlockIndex::isSolid() const
{
	return m_solid != nullptr;
}
const MaterialType& BlockIndex::getSolidMaterial() const
{
	return *m_solid;
}
bool BlockIndex::canSeeIntoFromAlways(const BlockIndex& block) const
{
	if(isSolid() && !getSolidMaterial().transparent)
		return false;
	if(m_hasBlockFeatures.contains(BlockFeatureType::door))
		return false;
	// looking up.
	if(m_z > block.m_z)
	{
		const BlockFeature* floor = m_hasBlockFeatures.atConst(BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		if(m_hasBlockFeatures.contains(BlockFeatureType::hatch))
			return false;
	}
	// looking down.
	if(m_z < block.m_z)
	{
		const BlockFeature* floor = block.m_hasBlockFeatures.atConst(BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = block.m_hasBlockFeatures.atConst(BlockFeatureType::hatch);
		if(hatch != nullptr && !hatch->materialType->transparent)
			return false;
	}
	return true;
}
void BlockIndex::moveContentsTo(BlockIndex& block)
{
	if(isSolid())
	{
		const MaterialType& materialType = getSolidMaterial();
		setNotSolid();
		block.setSolid(materialType);
	}
	//TODO: other stuff falls?
}
BlockIndex* BlockIndex::offset(int32_t ax, int32_t ay, int32_t az) const
{
	ax += m_x;
	ay += m_y;
	az += m_z;
	if(ax < 0 || (DistanceInBlocks)ax >= m_area->m_sizeX || ay < 0 || (DistanceInBlocks)ay >= m_area->m_sizeY || az < 0 || (DistanceInBlocks)az >= m_area->m_sizeZ)
		return nullptr;
	return &m_area->getBlock(ax, ay, az);
}
BlockIndex& BlockIndex::offsetNotNull(int32_t ax, int32_t ay, int32_t az) const
{
	ax += m_x;
	ay += m_y;
	az += m_z;
	return m_area->getBlock(ax, ay, az);
}
std::array<int32_t, 3> BlockIndex::relativeOffsetTo(const BlockIndex& block) const
{
	return {(int)block.m_x - (int)m_x, (int)block.m_y - (int)m_y, (int)block.m_z - (int)m_z};
}
bool BlockIndex::canSeeThrough() const
{
	if(isSolid() && !getSolidMaterial().transparent)
		return false;
	const BlockFeature* door = m_hasBlockFeatures.atConst(BlockFeatureType::door);
	if(door != nullptr && !door->materialType->transparent && door->closed)
		return false;
	return true;
}
bool BlockIndex::canSeeThroughFloor() const
{
	const BlockFeature* floor = m_hasBlockFeatures.atConst(BlockFeatureType::floor);
	if(floor != nullptr && !floor->materialType->transparent)
		return false;
	const BlockFeature* hatch = m_hasBlockFeatures.atConst(BlockFeatureType::hatch);
	if(hatch != nullptr && !hatch->materialType->transparent && hatch->closed)
		return false;
	return true;
}
bool BlockIndex::canSeeThroughFrom(const BlockIndex& block) const
{
	if(!canSeeThrough())
		return false;
	// On the same level.
	if(m_z == block.m_z)
		return true;
	// looking up.
	if(m_z > block.m_z)
	{
		if(!canSeeThroughFloor())
			return false;
	}
	// looking down.
	if(m_z < block.m_z)
	{
		if(!block.canSeeThroughFloor())
			return false;
	}
	return true;
}
Facing BlockIndex::facingToSetWhenEnteringFrom(const BlockIndex& block) const
{
	if(block.m_x > m_x)
		return 3;
	if(block.m_x < m_x)
		return 1;
	if(block.m_y < m_y)
		return 2;
	return 0;
}
Facing BlockIndex::facingToSetWhenEnteringFromIncludingDiagonal(const BlockIndex& block, Facing inital) const
{
	if(m_x == block.m_x)
	{
		if(m_y < block.m_y)
			return 0; // North
		if(m_y == block.m_y)
			return inital; // Up or Down
		if(m_y > block.m_y)
			return 4; // South
	}
	else if(m_x < block.m_x)
	{
		if(m_y > block.m_y)
			return 7; // North West
		if(m_y == block.m_y)
			return 6; // West
		if(m_y < block.m_y)
			return 5;// South West
	}
	assert(m_x > block.m_x);
	if(m_y > block.m_y)
		return 3; // South East
	if(m_y == block.m_y)
		return 2; // East
	if(m_y < block.m_y)
		return 1;// North East
	assert(false);
	return UINT8_MAX;
}
bool BlockIndex::isSupport() const
{
	return isSolid() || m_hasBlockFeatures.isSupport();
}
bool BlockIndex::hasLineOfSightTo(BlockIndex& block) const
{
	return m_area->m_hasActors.m_opacityFacade.hasLineOfSight(*this, block);
}
bool BlockIndex::operator==(const BlockIndex& block) const { return &block == this; };
//TODO: Replace with cuboid.
std::vector<BlockIndex*> BlockIndex::selectBetweenCorners(BlockIndex* otherBlock) const
{
	assert(otherBlock->m_x < m_area->m_sizeX);
	assert(otherBlock->m_y < m_area->m_sizeY);
	assert(otherBlock->m_z < m_area->m_sizeZ);
	std::vector<BlockIndex*> output;
	DistanceInBlocks minX = std::min(m_x, otherBlock->m_x);
	DistanceInBlocks maxX = std::max(m_x, otherBlock->m_x);
	DistanceInBlocks minY = std::min(m_y, otherBlock->m_y);
	DistanceInBlocks maxY = std::max(m_y, otherBlock->m_y);
	DistanceInBlocks minZ = std::min(m_z, otherBlock->m_z);
	DistanceInBlocks maxZ = std::max(m_z, otherBlock->m_z);
	for(DistanceInBlocks x = minX; x <= maxX; ++x)
		for(DistanceInBlocks y = minY; y <= maxY; ++y)
			for(DistanceInBlocks z = minZ; z <= maxZ; ++z)
			{
				output.push_back(&m_area->getBlock(x, y, z));
				assert(output.back() != nullptr);
			}
	return output;
}
std::unordered_set<BlockIndex*> BlockIndex::collectAdjacentsInRange(DistanceInBlocks range)
{
	auto condition = [&](BlockIndex& b){ return b.taxiDistance(*this) <= range; };
	return collectAdjacentsWithCondition(condition);
}
std::vector<BlockIndex*> BlockIndex::collectAdjacentsInRangeVector(DistanceInBlocks range)
{
	auto result = collectAdjacentsInRange(range);
	std::vector<BlockIndex*> output(result.begin(), result.end());
	return output;
}
void BlockIndex::loadFromJson(Json data, DeserializationMemo& deserializationMemo, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_exposedToSky = data.contains("e");
	m_underground = data.contains("u");
	m_outdoors = data.contains("o");
	m_visible = data.contains("v");
	if(data.contains("reservable"))
		deserializationMemo.m_reservables[data["reservable"].get<uintptr_t>()] = &m_reservable;
	if(data.contains("s"))
	{
		const MaterialType& materialType = MaterialType::byName(data["s"].get<std::string>());
		setSolid(materialType);
	}
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
	if(data.contains("f"))
		m_hasFluids.load(data["f"], deserializationMemo);
}
Json BlockIndex::toJson() const 
{
	//TODO: This format is far too verbose, encode with MessagePack?
	Json data{};
	if(m_visible)
		data["v"] = 1;
	if(m_exposedToSky)
		data["e"] = 1;
	if(m_underground)
		data["u"] = 1;
	if(m_outdoors)
		data["o"] = 1;
	if(m_reservable.hasAnyReservations())
		data["reservable"] = reinterpret_cast<uintptr_t>(&m_reservable);
	if(m_solid != nullptr)
		data["s"] = m_solid->name;
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
	if(m_hasFluids.any())
		data["f"] = m_hasFluids.toJson();
	return data;
}
Json BlockIndex::positionToJson() const { return {{"x", m_x}, {"y", m_y}, {"z", m_z}, {"area", m_area->m_id}}; }
