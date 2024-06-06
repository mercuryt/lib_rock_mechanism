#include "blocks.h"
#include "../area.h"
#include "../materialType.h"
#include "../types.h"
#include "../blockFeature.h"
#include "../locationBuckets.h"
#include "../fluidType.h"

Blocks::Blocks(Area& area, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z) : m_area(area), m_sizeX(x), m_sizeY(y), m_sizeZ(z)
{
	BlockIndex count = x * y * z;
	resize(count);
	for(BlockIndex i = 0; i < count; ++i)
		initalize(i);
	m_offsetsForAdjacentCountTable = makeOffsetsForAdjacentCountTable();
}
void Blocks::resize(BlockIndex count)
{
	m_materialType.resize(count);
	m_features.resize(count);
	m_fluid.resize(count);
	m_mist.resize(count);
	m_mistInverseDistanceFromSource.resize(count);
	m_totalFluidVolume.resize(count);
	m_actors.resize(count);
	m_items.resize(count);
	m_plants.resize(count);
	m_shapes.resize(count);
	m_dynamicVolume.resize(count);
	m_staticVolume.resize(count);
	m_projects.resize(count);
	m_fires.resize(count);
	m_temperatureDelta.resize(count);
	m_locationBucket.resize(count);
	m_directlyAdjacent.resize(count);
	m_exposedToSky.resize(count);
	m_underground.resize(count);
	m_isEdge.resize(count);
	m_outdoors.resize(count);
	m_visible.resize(count);
	m_constructed.resize(count);
}
// Create from scratch, not load from json.
void Blocks::initalize(BlockIndex index)
{
	// Default initalization is fine for most things.
	m_exposedToSky[index] = true;
	Point3D point = getCoordinates(index);
	m_isEdge[index] = (
		point.x == 0 || point.x == (m_sizeX - 1) || 
		point.y == 0 || point.y == (m_sizeY - 1) || 
		point.z == 0 || point.z == (m_sizeZ - 1) 
	);
	m_outdoors[index] = true;
	recordAdjacent(index);
}
BlockIndex Blocks::offset(BlockIndex index, int32_t ax, int32_t ay, int32_t az) const
{
	Point3D coordinates = getCoordinates(index);
	ax += coordinates.x;
	ay += coordinates.y;
	az += coordinates.z;
	if(ax < 0 || (DistanceInBlocks)ax >= m_sizeX || ay < 0 || (DistanceInBlocks)ay >= m_sizeY || az < 0 || (DistanceInBlocks)az >= m_sizeZ)
		return BLOCK_INDEX_MAX;
	return getIndex({(DistanceInBlocks)ax, (DistanceInBlocks)ay, (DistanceInBlocks)az});
}
void Blocks::load(const Json& data, DeserializationMemo&)
{
	m_materialType = data["solid"];
	for(const Json& pair : data["features"])
		m_features[pair[0]] = pair[1].get<std::vector<BlockFeature>>();
	for(const Json& pair : data["fluid"])
		fluid_add(pair[0].get<BlockIndex>(), pair[1]["volume"], FluidType::byName(pair[1]["fluidType"].get<std::string>()));
	for(const Json& pair : data["mist"])
		m_mist[pair[0]] = &FluidType::byName(pair[1].get<std::string>());
	for(const Json& pair : data["mistInverseDistanceFromSource"])
		m_mistInverseDistanceFromSource[pair[0]] = pair[1].get<DistanceInBlocks>();

}
Json Blocks::toJson() const
{
	Json output{
		{"solid", m_materialType},
		{"features", Json()},
		{"fluid", Json()},
		{"mist", Json()},
		{"mistInverseDistanceFromSource", Json()}
	};
	for(size_t i = 0; i < m_materialType.size(); ++i)
	{
		output["features"][i] = m_features.at(i);
		output["fluid"][i] = m_fluid.at(i);
		output["mist"][i] = m_mist.at(i);
		output["mistInverseDistanceFromSource"][i] = m_mistInverseDistanceFromSource.at(i);
	}
	return output;
}
Cuboid Blocks::getAll()
{
	return Cuboid(*this, size() - 1, 0);
}
const Cuboid Blocks::getAll() const
{
	return const_cast<Blocks*>(this)->getAll();
}
size_t Blocks::size() const
{
	return m_materialType.size();
}
BlockIndex Blocks::getIndex(Point3D coordinates) const
{
	return coordinates.x + (coordinates.y * m_sizeX) + (coordinates.z * m_sizeY * m_sizeX);
}
BlockIndex Blocks::getIndex(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z) const
{
	return getIndex({x,y,z});
}
Point3D Blocks::getCoordinates(BlockIndex index) const
{
	Point3D output;
	output.z = index / (m_sizeX * m_sizeY);
	index -= output.z * m_sizeX * m_sizeY;
	output.y = index / m_sizeX;
	output.x = index - (output.y * m_sizeX);
	return output;
}
DistanceInBlocks Blocks::getZ(BlockIndex index) const
{
	return index / (m_sizeX * m_sizeY);
}
BlockIndex Blocks::getAtFacing(BlockIndex index, Facing facing) const
{
	return m_directlyAdjacent.at(index).at(facing);
}
BlockIndex Blocks::getCenterAtGroundLevel() const
{
	BlockIndex center = getIndex({m_sizeX / 2, m_sizeY  / 2, m_sizeZ - 1});
	BlockIndex below = getBlockBelow(center);
	while(!solid_is(below))
	{
		center = getBlockBelow(center);
		below = getBlockBelow(center);
	}
	return center;
}
Cuboid Blocks::getZLevel(DistanceInBlocks z)
{
	return Cuboid(*this, getIndex({m_sizeX - 1, m_sizeY - 1, z}), getIndex({0, 0, z}));
}
LocationBucket& Blocks::getLocationBucket(BlockIndex index)
{
	assert(m_locationBucket.at(index));
	return *m_locationBucket.at(index);
}
void Blocks::recordAdjacent(BlockIndex index)
{
	static const int32_t offsetsList[6][3] = {{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}};
	for(uint32_t i = 0; i < 6; i++)
	{
		auto& offsets = offsetsList[i];
		m_directlyAdjacent[index][i] = offset(index, offsets[0],offsets[1],offsets[2]);
	}
}
void Blocks::assignLocationBuckets()
{
	for(BlockIndex index : getAll())
		m_locationBucket[index] = &m_area.m_hasActors.m_locationBuckets.getBucketFor(index);
}
const std::array<BlockIndex, 6>& Blocks::getDirectlyAdjacent(BlockIndex index) const
{
	assert(m_directlyAdjacent.size() > index);
	return m_directlyAdjacent[index];
}
BlockIndex Blocks::getBlockBelow(BlockIndex index) const 
{
	return m_directlyAdjacent.at(index)[0];
}
BlockIndex Blocks::getBlockAbove(BlockIndex index) const
{
	return m_directlyAdjacent.at(index)[5];
}
BlockIndex Blocks::getBlockNorth(BlockIndex index) const
{
	return m_directlyAdjacent.at(index)[3];
}
BlockIndex Blocks::getBlockWest(BlockIndex index) const
{
	return m_directlyAdjacent.at(index)[2];
}
BlockIndex Blocks::getBlockSouth(BlockIndex index) const
{
	return m_directlyAdjacent.at(index)[1];
}
BlockIndex Blocks::getBlockEast(BlockIndex index) const
{
	return m_directlyAdjacent.at(index)[4];
}
std::vector<BlockIndex> Blocks::getAdjacentWithEdgeAdjacent(BlockIndex index) const
{
	std::vector<BlockIndex> output;
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
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
// TODO: cache.
std::vector<BlockIndex> Blocks::getAdjacentWithEdgeAndCornerAdjacent(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(26);
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsListAllAdjacent[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getAdjacentWithEdgeAndCornerAdjacentUnfiltered(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(26);
	for(uint32_t i = 0; i < 26; i++)
	{
		auto& offsets = offsetsListAllAdjacent[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getEdgeAdjacentOnly(BlockIndex index) const
{
	std::vector<BlockIndex> output;
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
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getEdgeAdjacentOnSameZLevelOnly(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getEdgeAdjacentOnlyOnNextZLevelDown(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getEdgeAndCornerAdjacentOnlyOnNextZLevelDown(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,-1,-1}, {-1,0,-1}, {-1, 1, -1},
		{0,-1,-1}, {0,1,-1},
		{1,-1,-1}, {1,0,-1}, {1,1,-1}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getEdgeAdjacentOnlyOnNextZLevelUp(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(12);
	static const int32_t offsetsList[12][3] = {
		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	};
	for(uint32_t i = 0; i < 12; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getEdgeAndCornerAdjacentOnly(BlockIndex index) const
{
	std::vector<BlockIndex> output;
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
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
std::vector<BlockIndex> Blocks::getAdjacentOnSameZLevelOnly(BlockIndex index) const
{
	std::vector<BlockIndex> output;
	output.reserve(4);
	static const int32_t offsetsList[4][3] = {
		{-1,0,0}, {1,0,0}, 
		{0,-1,0}, {0,1,0}
	};
	for(uint32_t i = 0; i < 4; i++)
	{
		auto& offsets = offsetsList[i];
		BlockIndex block = offset(index, offsets[0],offsets[1],offsets[2]);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
DistanceInBlocks Blocks::distance(BlockIndex index, BlockIndex otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	DistanceInBlocks dx = abs((int)coordinates.x - (int)otherCoordinates.x);
	DistanceInBlocks dy = abs((int)coordinates.y - (int)otherCoordinates.y);
	DistanceInBlocks dz = abs((int)coordinates.z - (int)otherCoordinates.z);
	return pow((pow(dx, 2) + pow(dy, 2) + pow(dz, 2)), 0.5);
}
DistanceInBlocks Blocks::taxiDistance(BlockIndex index, BlockIndex otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return 
		abs((int)coordinates.x - (int)otherCoordinates.x) + 
		abs((int)coordinates.y - (int)otherCoordinates.y) + 
		abs((int)coordinates.z - (int)otherCoordinates.z);
}
bool Blocks::squareOfDistanceIsMoreThen(BlockIndex index, BlockIndex otherIndex, DistanceInBlocks distanceCubed) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	DistanceInBlocks dx = abs((int32_t)otherCoordinates.x - (int32_t)coordinates.x);
	DistanceInBlocks dy = abs((int32_t)otherCoordinates.y - (int32_t)coordinates.y);
	DistanceInBlocks dz = abs((int32_t)otherCoordinates.z - (int32_t)coordinates.z);
	return (dx * dx) + (dy * dy) + (dz * dz) > distanceCubed;
}
bool Blocks::isAdjacentToAny(BlockIndex index, std::unordered_set<BlockIndex>& blocks) const
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent != BLOCK_INDEX_MAX && blocks.contains(adjacent))
			return true;
	return false;
}
bool Blocks::isAdjacentTo(BlockIndex index, BlockIndex otherIndex) const
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent != BLOCK_INDEX_MAX && otherIndex == adjacent)
			return true;
	return false;
}
bool Blocks::isAdjacentToIncludingCornersAndEdges(BlockIndex index, BlockIndex otherIndex) const
{
	const auto& adjacents = getAdjacentWithEdgeAndCornerAdjacent(index);
	return std::ranges::find(adjacents, otherIndex) != adjacents.end();
}
bool Blocks::isAdjacentTo(BlockIndex index, HasShape& hasShape) const
{
	return hasShape.getAdjacentBlocks().contains(index);
}
void Blocks::setExposedToSky(BlockIndex index, bool exposed)
{
	m_exposedToSky[index] = exposed;
	plant_updateGrowingStatus(index);
}
void Blocks::setBelowExposedToSky(BlockIndex index)
{
	BlockIndex block = getBlockBelow(index);
	while(block != BLOCK_INDEX_MAX && canSeeThroughFrom(block, getBlockAbove(block)) && !m_exposedToSky[block])
	{
		setExposedToSky(block, true);
		block = getBlockBelow(block);
	}
}
void Blocks::setBelowVisible(BlockIndex index)
{
	BlockIndex block = getBlockBelow(index);
	while(block != BLOCK_INDEX_MAX && canSeeThroughFrom(block, getBlockAbove(block)) && !m_visible[block])
	{
		m_visible[block] = true;
		block = getBlockBelow(block);
	}
}
void Blocks::setBelowNotExposedToSky(BlockIndex index)
{
	BlockIndex block = getBlockBelow(index);
	while(block != BLOCK_INDEX_MAX && m_exposedToSky[block])
	{
		m_exposedToSky[block] = false;
		block = getBlockBelow(block);
	}
}
void Blocks::solid_set(BlockIndex index, const MaterialType& materialType, bool constructed)
{
	assert(m_items[index].empty());
	if(m_plants.at(index))
	{
		assert(!m_plants.at(index)->m_plantSpecies.isTree);
		m_plants.at(index)->die();
	}
	if(&materialType == m_materialType[index])
		return;
	bool wasEmpty = m_materialType[index] == nullptr;
	m_materialType[index] = &materialType;
	m_constructed[index] = constructed;
	fluid_onBlockSetSolid(index);
	m_visible[index] = false;
	// Opacity.
	if(!materialType.transparent && wasEmpty)
		m_area.m_hasActors.m_visionCuboids.blockIsSometimesOpaque(index);
	m_area.m_hasActors.m_opacityFacade.update(index);
	// Set blocks below as not exposed to sky.
	setExposedToSky(index, false);
	setBelowNotExposedToSky(index);
	// Remove from stockpiles.
	m_area.m_hasStockPiles.removeBlockFromAllFactions(index);
	m_area.m_hasCraftingLocationsAndJobs.maybeRemoveLocation(index);
	if(wasEmpty && m_reservables.contains(index))
		// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid block.
		m_reservables.at(index).clearAll();
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::solid_setNot(BlockIndex index)
{
	if(!solid_is(index))
		return;
	m_materialType[index] = nullptr;
	m_constructed[index] = false;
	fluid_onBlockSetNotSolid(index);
	m_area.m_hasActors.m_visionCuboids.blockIsNeverOpaque(index);
	m_area.m_hasActors.m_opacityFacade.update(index);
	if(getBlockAbove(index) == BLOCK_INDEX_MAX || m_exposedToSky[getBlockAbove(index)])
	{
		setExposedToSky(index, true);
		setBelowExposedToSky(index);
	}
	//TODO: Check if any adjacent are visible first?
	m_visible[index] = true;
	setBelowVisible(index);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid block.
	m_reservables.erase(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
const MaterialType& Blocks::solid_get(BlockIndex index) const
{
	return *m_materialType.at(index);
}
bool Blocks::solid_is(BlockIndex index) const
{
	return m_materialType.at(index);
}
Mass Blocks::getMass(BlockIndex index) const
{
	assert(solid_is(index));
	return m_materialType.at(index)->density * Config::maxBlockVolume;
}
bool Blocks::canSeeIntoFromAlways(BlockIndex to, BlockIndex from) const
{
	if(solid_is(to) && !m_materialType[to]->transparent)
		return false;
	if(blockFeature_contains(to, BlockFeatureType::door))
		return false;
	// looking up.
	if(getZ(to) > getZ(from))
	{
		const BlockFeature* floor = blockFeature_atConst(to, BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = blockFeature_atConst(to, BlockFeatureType::hatch);
		if(hatch != nullptr && !hatch->materialType->transparent)
			return false;
	}
	// looking down.
	if(getZ(to) < getZ(from))
	{
		const BlockFeature* floor = blockFeature_atConst(from, BlockFeatureType::floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = blockFeature_atConst(from, BlockFeatureType::hatch);
		if(hatch != nullptr && !hatch->materialType->transparent)
			return false;
	}
	return true;
}
void Blocks::moveContentsTo(BlockIndex from, BlockIndex to)
{
	if(solid_is(from))
	{
		solid_set(to, solid_get(from), m_constructed[from]);
		solid_setNot(from);
	}
	//TODO: other stuff falls?
}
BlockIndex Blocks::offsetNotNull(BlockIndex index, int32_t ax, int32_t ay, int32_t az) const
{
	Point3D coordinates = getCoordinates(index);
	assert((int)coordinates.x + ax >= 0);
	assert((int)coordinates.y + ay >= 0);
	assert((int)coordinates.z + az >= 0);
	coordinates.x += ax;
	coordinates.y += ay;
	coordinates.z += az;
	return getIndex(coordinates);
}
BlockIndex Blocks::indexAdjacentToAtCount(BlockIndex index, uint8_t adjacentCount) const
{
	// If block is on edge check for the offset being beyond the area. If so return BLOCK_INDEX_MAX.
	if(m_isEdge[index])
	{
		Point3D coordinates = getCoordinates(index);
		auto [x, y, z] = Blocks::offsetsListAllAdjacent[adjacentCount];
		Vector3D vector(x, y, z);
		if((vector.x == -1 && coordinates.x == 0) || (vector.x == 1 && coordinates.x == m_sizeX - 1))
			return BLOCK_INDEX_MAX;
		if((vector.y == -1 && coordinates.y == 0) || (vector.y == 1 && coordinates.y == m_sizeY - 1))
			return BLOCK_INDEX_MAX;
		if((vector.z == -1 && coordinates.z == 0) || (vector.z == 1 && coordinates.z == m_sizeZ - 1))
			return BLOCK_INDEX_MAX;
	}
	return index + m_offsetsForAdjacentCountTable[adjacentCount];
}
std::array<int, 26> Blocks::makeOffsetsForAdjacentCountTable() const
{
	std::array<int, 26> output;
	uint i = 0;
	for(auto [x, y, z] : offsetsListAllAdjacent)
		output[i++] = (z * m_sizeY * m_sizeX) + (y * m_sizeX) + x;
	return output;
}
std::array<int32_t, 3> Blocks::relativeOffsetTo(BlockIndex index, BlockIndex otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return {
		(int)otherCoordinates.x - (int)coordinates.x, 
		(int)otherCoordinates.y - (int)coordinates.y, 
		(int)otherCoordinates.z - (int)coordinates.z
	};
}
bool Blocks::canSeeThrough(BlockIndex index) const
{
	if(solid_is(index) && !solid_get(index).transparent)
		return false;
	const BlockFeature* door = blockFeature_atConst(index, BlockFeatureType::door);
	if(door != nullptr && !door->materialType->transparent && door->closed)
		return false;
	return true;
}
bool Blocks::canSeeThroughFloor(BlockIndex index) const
{
	const BlockFeature* floor = blockFeature_atConst(index, BlockFeatureType::floor);
	if(floor != nullptr && !floor->materialType->transparent)
		return false;
	const BlockFeature* hatch = blockFeature_atConst(index, BlockFeatureType::hatch);
	if(hatch != nullptr && !hatch->materialType->transparent && hatch->closed)
		return false;
	return true;
}
bool Blocks::canSeeThroughFrom(BlockIndex index, BlockIndex otherIndex) const
{
	if(!canSeeThrough(index))
		return false;
	// On the same level.
	auto z = getZ(index);
	auto otherZ = getZ(otherIndex);
	if(z == otherZ)
		return true;
	// looking up.
	if(z > otherZ)
	{
		if(!canSeeThroughFloor(index))
			return false;
	}
	// looking down.
	if(z < otherZ)
	{
		if(!canSeeThroughFloor(otherIndex))
			return false;
	}
	return true;
}
Facing Blocks::facingToSetWhenEnteringFrom(BlockIndex index, BlockIndex otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	if(otherCoordinates.x > coordinates.x)
		return 3;
	if(otherCoordinates.x < coordinates.x)
		return 1;
	if(otherCoordinates.y < coordinates.y)
		return 2;
	return 0;
}
Facing Blocks::facingToSetWhenEnteringFromIncludingDiagonal(BlockIndex index, BlockIndex otherIndex, Facing inital) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	if(coordinates.x == otherCoordinates.x)
	{
		if(coordinates.y < otherCoordinates.y)
			return 0; // North
		if(coordinates.y == otherCoordinates.y)
			return inital; // Up or Down
		if(coordinates.y > otherCoordinates.y)
			return 4; // South
	}
	else if(coordinates.x < otherCoordinates.x)
	{
		if(coordinates.y > otherCoordinates.y)
			return 7; // North West
		if(coordinates.y == otherCoordinates.y)
			return 6; // West
		if(coordinates.y < otherCoordinates.y)
			return 5;// South West
	}
	assert(coordinates.x > otherCoordinates.x);
	if(coordinates.y > otherCoordinates.y)
		return 3; // South East
	if(coordinates.y == otherCoordinates.y)
		return 2; // East
	if(coordinates.y < otherCoordinates.y)
		return 1;// North East
	assert(false);
	return UINT8_MAX;
}
bool Blocks::isSupport(BlockIndex index) const
{
	return solid_is(index) || blockFeature_isSupport(index);
}
bool Blocks::isOutdoors(BlockIndex index) const
{
	return m_outdoors[index];
}
bool Blocks::isUnderground(BlockIndex index) const
{
	return m_underground[index];
}
bool Blocks::isExposedToSky(BlockIndex index) const
{
	return m_exposedToSky[index];
}
bool Blocks::isEdge(BlockIndex index) const
{
	return m_isEdge[index];
}
bool Blocks::hasLineOfSightTo(BlockIndex index, BlockIndex other) const
{
	return m_area.m_hasActors.m_opacityFacade.hasLineOfSight(index, other);
}
std::unordered_set<BlockIndex> Blocks::collectAdjacentsInRange(BlockIndex index, DistanceInBlocks range)
{
	auto condition = [&](BlockIndex b){ return taxiDistance(b, index) <= range; };
	return collectAdjacentsWithCondition(index, condition);
}
std::vector<BlockIndex> Blocks::collectAdjacentsInRangeVector(BlockIndex index, DistanceInBlocks range)
{
	auto result = collectAdjacentsInRange(index, range);
	std::vector<BlockIndex> output(result.begin(), result.end());
	return output;
}
