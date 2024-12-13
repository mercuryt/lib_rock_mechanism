#include "blocks.h"
#include "../area.h"
#include "../materialType.h"
#include "../types.h"
#include "../blockFeature.h"
#include "../locationBuckets.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../plants.h"
#include <string>

Blocks::Blocks(Area& area, const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) : m_area(area), m_sizeX(x), m_sizeY(y), m_sizeZ(z)
{
	assert(!area.m_loaded);
	BlockIndex count = BlockIndex::create((x * y * z).get());
	resize(count);
	for(BlockIndex i = BlockIndex::create(0); i < count; ++i)
		initalize(i);
	m_offsetsForAdjacentCountTable = makeOffsetsForAdjacentCountTable();
}
void Blocks::resize(const BlockIndex& count)
{
	m_reservables.resize(count);
	m_materialType.resize(count);
	m_features.resize(count);
	m_fluid.resize(count);
	m_mist.resize(count);
	m_mistInverseDistanceFromSource.resize(count);
	m_totalFluidVolume.resize(count);
	m_actorVolume.resize(count);
	m_itemVolume.resize(count);
	m_actors.resize(count);
	m_items.resize(count);
	m_plants.resize(count);
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
void Blocks::initalize(const BlockIndex& index)
{
	// Default initalization is fine for most things.
	m_exposedToSky.set(index);
	Point3D point = getCoordinates(index);
	m_totalFluidVolume[index] = CollisionVolume::create(0);
	m_dynamicVolume[index] = CollisionVolume::create(0);
	m_staticVolume[index] = CollisionVolume::create(0);
	m_temperatureDelta[index] = TemperatureDelta::create(0);
	m_isEdge.set(index, (
		point.x == 0 || point.x == (m_sizeX - 1) || 
		point.y == 0 || point.y == (m_sizeY - 1) || 
		point.z == 0 || point.z == (m_sizeZ - 1) 
	));
	m_outdoors.set(index);
	recordAdjacent(index);
}
BlockIndex Blocks::offset(const BlockIndex& index, int32_t ax, int32_t ay, int32_t az) const
{
	Point3D coordinates = getCoordinates(index);
	ax += coordinates.x.get();
	ay += coordinates.y.get();
	az += coordinates.z.get();
	if(ax < 0 || ax >= m_sizeX || ay < 0 || ay >= m_sizeY || az < 0 || az >= m_sizeZ)
		return BlockIndex::null();
	return getIndex(DistanceInBlocks::create(ax), DistanceInBlocks::create(ay), DistanceInBlocks::create(az));
}
void Blocks::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(auto& [key, value] : data["solid"].items())
		m_materialType[BlockIndex::create(std::stoi(key))] = value.get<MaterialTypeId>();
	for(auto& [key, value] : data["features"].items())
		m_features[BlockIndex::create(std::stoi(key))] = value.get<std::vector<BlockFeature>>();
	for(auto& [key, value] : data["fluid"].items())
		for(const Json& fluidData : value)
			fluid_add(BlockIndex::create(std::stoi(key)), fluidData["volume"].get<CollisionVolume>(), fluidData["type"].get<FluidTypeId>());
	for(auto& [key, value] : data["mist"].items())
		m_mist[BlockIndex::create(std::stoi(key))] = FluidType::byName(value.get<std::string>());
	for(auto& [key, value] : data["mistInverseDistanceFromSource"].items())
		m_mistInverseDistanceFromSource[BlockIndex::create(std::stoi(key))] = value.get<DistanceInBlocks>();
	for(auto& [key, value] : data["reservables"].items())
	{
		auto& reservable = m_reservables[BlockIndex::create(std::stoi(key))] = std::make_unique<Reservable>(Quantity::create(1u));
		deserializationMemo.m_reservables[value.get<uintptr_t>()] = reservable.get();
	}
}
Json Blocks::toJson() const
{
	Json output{
		{"x", m_sizeX},
		{"y", m_sizeY},
		{"z", m_sizeZ},
		{"solid", Json::object()},
		{"features", Json::object()},
		{"fluid", Json::object()},
		{"mist", Json::object()},
		{"mistInverseDistanceFromSource", Json::object()},
		{"reservables", Json::object()},
	};
	for(BlockIndex i = BlockIndex::create(0); i < m_materialType.size(); ++i)
	{
		std::string strI = std::to_string(i.get());
		if(!blockFeature_empty(i))
			output["features"][strI] = m_features[i];
		if(fluid_any(i))
			output["fluid"][strI] = m_fluid[i];
		if(m_mist[i].exists())
			output["mist"][strI] = m_mist[i];
		if(m_mistInverseDistanceFromSource[i] != 0)
			output["mistInverseDistanceFromSource"][strI] = m_mistInverseDistanceFromSource[i];
		if(m_reservables[i] != nullptr)
			output["reservables"][strI] = reinterpret_cast<uintptr_t>(&*m_reservables[i]);
		if(m_materialType[i].exists())
			output["solid"][strI] = m_materialType[i];
	}
	return output;
}
Cuboid Blocks::getAll()
{
	return Cuboid(*this, BlockIndex::create(size() - 1), BlockIndex::create(0));
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
	assert(coordinates.x < m_sizeX);
	assert(coordinates.y < m_sizeY);
	assert(coordinates.z < m_sizeZ);
	return BlockIndex::create((coordinates.x + (coordinates.y * m_sizeX) + (coordinates.z * m_sizeY * m_sizeX)).get());
}
BlockIndex Blocks::maybeGetIndexFromOffsetOnEdge(Point3D coordinates, const std::array<int8_t, 3> offset) const
{
	if(
		!(// Invert condition to put the common path in the if rather then else, as a hint to branch predictor.
			(coordinates.x == 0 && offset[0] == -1) ||
			(coordinates.y == 0 && offset[1] == -1) ||
			(coordinates.z == 0 && offset[2] == -1) ||
			(coordinates.x == m_sizeX - 1 && offset[0] == 1) ||
			(coordinates.y == m_sizeY - 1 && offset[1] == 1) ||
			(coordinates.z == m_sizeZ - 1 && offset[2] == 1)
		)
	)
		return BlockIndex::create((
			coordinates.x + offset[0]  +
			((coordinates.y + offset[1]) * m_sizeX) +
			((coordinates.z + offset[2]) * m_sizeY * m_sizeX)
		).get());
	else
		return BlockIndex::null();
}
BlockIndex Blocks::getIndex_i(uint x, uint y, uint z) const
{
	return getIndex(DistanceInBlocks::create(x), DistanceInBlocks::create(y), DistanceInBlocks::create(z));
}
BlockIndex Blocks::getIndex(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) const
{
	return getIndex({x,y,z});
}
Point3D Blocks::getCoordinates(BlockIndex index) const
{
	Point3D output;
	output.z = DistanceInBlocks::create(index.get()) / (m_sizeX * m_sizeY);
	index -= BlockIndex::create((output.z * m_sizeX * m_sizeY).get());
	output.y = DistanceInBlocks::create(index.get()) / m_sizeX;
	output.x = DistanceInBlocks::create(index.get()) - (output.y * m_sizeX);
	return output;
}
Point3D_fractional Blocks::getCoordinatesFractional(const BlockIndex& index) const
{
	Point3D coordinates = getCoordinates(index);
	return {coordinates.x.toFloat(), coordinates.y.toFloat(), coordinates.z.toFloat()}; 
}
DistanceInBlocks Blocks::getZ(const BlockIndex& index) const
{
	return DistanceInBlocks::create(index.get()) / (m_sizeX * m_sizeY);
}
BlockIndex Blocks::getAtFacing(const BlockIndex& index, const Facing& facing) const
{
	return m_directlyAdjacent[index][facing.get()];
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
Cuboid Blocks::getZLevel(const DistanceInBlocks& z)
{
	return Cuboid(*this, getIndex({m_sizeX - 1, m_sizeY - 1, z}), getIndex({DistanceInBlocks::create(0), DistanceInBlocks::create(0), z}));
}
LocationBucket& Blocks::getLocationBucket(const BlockIndex& index)
{
	assert(m_locationBucket[index] != nullptr);
	return *m_locationBucket[index];
}
template<uint size, bool filter>
auto getAdjacentWithOffsets(const Blocks& blocks, const BlockIndex& index, const std::array<int8_t, 3>* offsetList) -> std::array<BlockIndex, size>
{
	std::array<BlockIndex, size> output;
	Point3D coordinates = blocks.getCoordinates(index);
	if(!blocks.isEdge(index))
		for(uint i = 0; i < size; i++)
		{
			const auto& offsets = *(offsetList + i);
			output[i] = blocks.getIndex(coordinates.x + offsets[0], coordinates.y + offsets[1], coordinates.z + offsets[2]);
		}
	else
		if constexpr(filter)
		{
			uint outputPosition = 0;
			for(uint i = 0; i < size; i++)
			{
				const auto& offsets = *(offsetList + i);
				BlockIndex block = blocks.maybeGetIndexFromOffsetOnEdge(coordinates, offsets);
				if(block.exists())
				{
					output[outputPosition] = block;
					++outputPosition;
				}
			}
		}
		else
		{
			for(uint i = 0; i < size; i++)
			{
				const auto& offsets = *(offsetList + i);
				BlockIndex block = blocks.maybeGetIndexFromOffsetOnEdge(coordinates, offsets);
				output[i] = block;
			}
		}
	return output;
}
void Blocks::recordAdjacent(const BlockIndex& index)
{
	static constexpr std::array<std::array<int8_t, 3>, 6> offsetsList{{{0,0,-1}, {0,-1,0}, {-1,0,0}, {0,1,0}, {1,0,0}, {0,0,1}}};
	m_directlyAdjacent[index] = getAdjacentWithOffsets<6, false>(*this, index, &offsetsList.front());
}
void Blocks::assignLocationBuckets()
{
	for(BlockIndex index : getAll())
		m_locationBucket[index] = &m_area.m_locationBuckets.getBucketFor(index);
}
const std::array<BlockIndex, 6>& Blocks::getDirectlyAdjacent(const BlockIndex& index) const
{
	assert(m_directlyAdjacent.size() > index);
	return m_directlyAdjacent[index];
}
BlockIndex Blocks::getBlockBelow(const BlockIndex& index) const 
{
	return m_directlyAdjacent[index][0];
}
BlockIndex Blocks::getBlockAbove(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][5];
}
BlockIndex Blocks::getBlockNorth(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][3];
}
BlockIndex Blocks::getBlockWest(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][2];
}
BlockIndex Blocks::getBlockSouth(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][1];
}
BlockIndex Blocks::getBlockEast(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][4];
}
BlockIndexArrayNotNull<18> Blocks::getAdjacentWithEdgeAdjacent(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 18>  offsetsList{{
		{-1,0,-1}, 
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,0,-1}, 

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {0,-1,0},

		{-1,0,1}, 
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,0,1}, 
	}};
	return BlockIndexArrayNotNull<18>(getAdjacentWithOffsets<18, true>(*this, index, offsetsList.begin()));
}
// TODO: cache.
BlockIndexArrayNotNull<26> Blocks::getAdjacentWithEdgeAndCornerAdjacent(const BlockIndex& index) const
{
	return getAdjacentWithOffsets<26, true>(*this, index, offsetsListAllAdjacent.begin());
}
std::array<BlockIndex, 26> Blocks::getAdjacentWithEdgeAndCornerAdjacentUnfiltered(const BlockIndex& index) const
{
	return getAdjacentWithOffsets<26, false>(*this, index, offsetsListAllAdjacent.begin());
}
BlockIndexArrayNotNull<12> Blocks::getEdgeAdjacentOnly(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 12>  offsetsList{{
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 

		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},

		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	}};
	return getAdjacentWithOffsets<12, true>(*this, index, offsetsList.begin());
}
BlockIndexArrayNotNull<4> Blocks::getEdgeAdjacentOnSameZLevelOnly(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 4>  offsetsList{{
		{-1,-1,0}, {1,1,0}, 
		{1,-1,0}, {-1,1,0},
	}};
	return getAdjacentWithOffsets<4, true>(*this, index, offsetsList.begin());
}
BlockIndexArrayNotNull<4> Blocks::getEdgeAdjacentOnlyOnNextZLevelDown(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 4>  offsetsList{{
		{-1,0,-1}, {0,-1,-1},
		{1,0,-1}, {0,1,-1}, 
	}};
	return getAdjacentWithOffsets<4, true>(*this, index, offsetsList.begin());
}
BlockIndexArrayNotNull<8> Blocks::getEdgeAndCornerAdjacentOnlyOnNextZLevelDown(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 8>  offsetsList{{
		{-1,-1,-1}, {-1,0,-1}, {-1, 1, -1},
		{0,-1,-1}, {0,1,-1},
		{1,-1,-1}, {1,0,-1}, {1,1,-1}
	}};
	return getAdjacentWithOffsets<8, true>(*this, index, offsetsList.begin());
}
BlockIndexArrayNotNull<4> Blocks::getEdgeAdjacentOnlyOnNextZLevelUp(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 4>  offsetsList{{
		{-1,0,1}, {0,-1,1},
		{0,1,1}, {1,0,1}, 
	}};
	return getAdjacentWithOffsets<4, true>(*this, index, offsetsList.begin());
}
BlockIndexArrayNotNull<20> Blocks::getEdgeAndCornerAdjacentOnly(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 20>  offsetsList{{
		{-1,-1,-1},
		{-1,-1,0},
		{-1,-1,1},
		{-1,0,-1},
		{-1,0,1},
		{-1,1,-1},
		{-1,1,0},
		{-1,1,1},
		{0,-1,-1},
		{0,-1,1},
		{0,1,-1},
		{0,1,1},
		{1,-1,-1},
		{1,-1,0},
		{1,-1,1},
		{1,0,-1},
		{1,0,1},
		{1,1,-1},
		{1,1,0},
		{1,1,1},
	}};
	return getAdjacentWithOffsets<20, true>(*this, index, offsetsList.begin());
}
BlockIndexArrayNotNull<4> Blocks::getAdjacentOnSameZLevelOnly(const BlockIndex& index) const
{
	static constexpr std::array<std::array<int8_t, 3>, 4>  offsetsList{{
		{-1,0,0}, {1,0,0}, 
		{0,-1,0}, {0,1,0}
	}};
	return getAdjacentWithOffsets<4, true>(*this, index, offsetsList.begin());
}
DistanceInBlocks Blocks::distance(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	return DistanceInBlocks::create(pow(distanceSquared(index, otherIndex).get(), 0.5));
}
DistanceInBlocks Blocks::taxiDistance(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return DistanceInBlocks::create(
		abs((int)coordinates.x.get() - (int)otherCoordinates.x.get()) + 
		abs((int)coordinates.y.get() - (int)otherCoordinates.y.get()) + 
		abs((int)coordinates.z.get() - (int)otherCoordinates.z.get())
	);
}
DistanceInBlocks Blocks::distanceSquared(const BlockIndex& index, const BlockIndex& other) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(other);
	DistanceInBlocks dx = DistanceInBlocks::create(abs((int)coordinates.x.get() - (int)otherCoordinates.x.get()));
	DistanceInBlocks dy = DistanceInBlocks::create(abs((int)coordinates.y.get() - (int)otherCoordinates.y.get()));
	DistanceInBlocks dz = DistanceInBlocks::create(abs((int)coordinates.z.get() - (int)otherCoordinates.z.get()));
	return DistanceInBlocks::create(pow(dx.get(), 2) + pow(dy.get(), 2) + pow(dz.get(), 2));
}
DistanceInBlocksFractional Blocks::distanceFractional(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D_fractional coordinates = getCoordinatesFractional(index);
	Point3D_fractional otherCoordinates = getCoordinatesFractional(otherIndex);
	DistanceInBlocksFractional dx = DistanceInBlocksFractional::create(std::abs(coordinates.x.get() - otherCoordinates.x.get()));
	DistanceInBlocksFractional dy = DistanceInBlocksFractional::create(std::abs(coordinates.y.get() - otherCoordinates.y.get()));
	DistanceInBlocksFractional dz = DistanceInBlocksFractional::create(std::abs(coordinates.z.get() - otherCoordinates.z.get()));
	return DistanceInBlocksFractional::create(std::pow((std::pow(dx.get(), 2) + std::pow(dy.get(), 2) + std::pow(dz.get(), 2)), 0.5));
}
bool Blocks::squareOfDistanceIsMoreThen(const BlockIndex& index, const BlockIndex& otherIndex, DistanceInBlocksFractional otherDistanceSquared) const
{
	return distanceSquared(index, otherIndex)> otherDistanceSquared.get();
}
bool Blocks::isAdjacentToAny(const BlockIndex& index, BlockIndices& blocks) const
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && blocks.contains(adjacent))
			return true;
	return false;
}
bool Blocks::isAdjacentTo(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && otherIndex == adjacent)
			return true;
	return false;
}
bool Blocks::isAdjacentToIncludingCornersAndEdges(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	const auto& adjacents = getAdjacentWithEdgeAndCornerAdjacent(index);
	return std::ranges::find(adjacents, otherIndex) != adjacents.end();
}
bool Blocks::isAdjacentToActor(const BlockIndex& index, const ActorIndex& actor) const
{
	return m_area.getActors().isAdjacentToLocation(actor, index);
}
void Blocks::setExposedToSky(const BlockIndex& index, bool exposed)
{
	m_exposedToSky.set(index, exposed);
	plant_updateGrowingStatus(index);
}
void Blocks::setBelowExposedToSky(const BlockIndex& index)
{
	BlockIndex block = getBlockBelow(index);
	while(block.exists() && canSeeThroughFrom(block, getBlockAbove(block)) && !m_exposedToSky[block])
	{
		setExposedToSky(block, true);
		plant_updateGrowingStatus(block);
		block = getBlockBelow(block);
	}
}
void Blocks::setBelowVisible(const BlockIndex& index)
{
	BlockIndex block = getBlockBelow(index);
	while(block.exists() && canSeeThroughFrom(block, getBlockAbove(block)) && !m_visible[block])
	{
		m_visible.set(block);
		block = getBlockBelow(block);
	}
}
void Blocks::setBelowNotExposedToSky(const BlockIndex& index)
{
	BlockIndex block = getBlockBelow(index);
	while(block.exists() && m_exposedToSky[block])
	{
		m_exposedToSky.unset(block);
		plant_updateGrowingStatus(block);
		block = getBlockBelow(block);
	}
}
void Blocks::solid_set(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed)
{
	assert(m_itemVolume[index].empty());
	Plants& plants = m_area.getPlants();
	if(m_plants[index].exists())
	{
		PlantIndex plant = m_plants[index];
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant)));
		plants.die(plant);
	}
	if(materialType == m_materialType[index])
		return;
	bool wasEmpty = m_materialType[index].empty();
	if(wasEmpty)
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(index);
	m_materialType[index] = materialType;
	m_constructed.set(index, constructed);
	fluid_onBlockSetSolid(index);
	m_visible.set(index);
	// Opacity.
	if(!MaterialType::getTransparent(materialType) && wasEmpty)
		m_area.m_visionCuboids.blockIsSometimesOpaque(index);
	m_area.m_opacityFacade.update(index);
	// Set blocks below as not exposed to sky.
	setExposedToSky(index, false);
	setBelowNotExposedToSky(index);
	// Remove from stockpiles.
	m_area.m_hasStockPiles.removeBlockFromAllFactions(index);
	m_area.m_hasCraftingLocationsAndJobs.maybeRemoveLocation(index);
	if(wasEmpty && m_reservables[index] != nullptr)
		// There are no reservations which can exist on both a solid and not solid block.
		dishonorAllReservations(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
}
void Blocks::solid_setNot(const BlockIndex& index)
{
	if(!solid_is(index))
		return;
	m_materialType[index].clear();
	m_constructed.unset(index);
	fluid_onBlockSetNotSolid(index);
	m_area.m_visionCuboids.blockIsNeverOpaque(index);
	m_area.m_opacityFacade.update(index);
	if(getBlockAbove(index).empty() || m_exposedToSky[getBlockAbove(index)])
	{
		setExposedToSky(index, true);
		setBelowExposedToSky(index);
	}
	//TODO: Check if any adjacent are visible first?
	m_visible.set(index);
	setBelowVisible(index);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid block.
	m_reservables[index] = nullptr;
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(index);
}
MaterialTypeId Blocks::solid_get(const BlockIndex& index) const
{
	return m_materialType[index];
}
bool Blocks::solid_is(const BlockIndex& index) const
{
	return m_materialType[index].exists();
}
Mass Blocks::getMass(const BlockIndex& index) const
{
	assert(solid_is(index));
	return MaterialType::getDensity(m_materialType[index]) * Volume::create(Config::maxBlockVolume.get());
}
bool Blocks::canSeeIntoFromAlways(const BlockIndex& to, const BlockIndex& from) const
{
	if(solid_is(to) && !MaterialType::getTransparent(m_materialType[to]))
		return false;
	if(blockFeature_contains(to, BlockFeatureType::door))
		return false;
	// looking up.
	if(getZ(to) > getZ(from))
	{
		const BlockFeature* floor = blockFeature_atConst(to, BlockFeatureType::floor);
		if(floor != nullptr && !MaterialType::getTransparent(floor->materialType))
			return false;
		const BlockFeature* hatch = blockFeature_atConst(to, BlockFeatureType::hatch);
		if(hatch != nullptr && !MaterialType::getTransparent(hatch->materialType))
			return false;
	}
	// looking down.
	if(getZ(to) < getZ(from))
	{
		const BlockFeature* floor = blockFeature_atConst(from, BlockFeatureType::floor);
		if(floor != nullptr && !MaterialType::getTransparent(floor->materialType))
			return false;
		const BlockFeature* hatch = blockFeature_atConst(from, BlockFeatureType::hatch);
		if(hatch != nullptr && !MaterialType::getTransparent(hatch->materialType))
			return false;
	}
	return true;
}
void Blocks::moveContentsTo(const BlockIndex& from, const BlockIndex& to)
{
	if(solid_is(from))
	{
		solid_set(to, solid_get(from), m_constructed[from]);
		solid_setNot(from);
	}
	//TODO: other stuff falls?
}
BlockIndex Blocks::offsetNotNull(const BlockIndex& index, int32_t ax, int32_t ay, int32_t az) const
{
	Point3D coordinates = getCoordinates(index);
	assert((int)coordinates.x.get() + ax >= 0);
	assert((int)coordinates.y.get() + ay >= 0);
	assert((int)coordinates.z.get() + az >= 0);
	coordinates.x += ax;
	coordinates.y += ay;
	coordinates.z += az;
	return getIndex(coordinates);
}
BlockIndex Blocks::indexAdjacentToAtCount(const BlockIndex& index, const AdjacentIndex& adjacentCount) const
{
	// If block is on edge check for the offset being beyond the area. If so return BlockIndex::null().
	if(m_isEdge[index])
	{
		Point3D coordinates = getCoordinates(index);
		auto [x, y, z] = Blocks::offsetsListAllAdjacent[adjacentCount.get()];
		Vector3D vector(x, y, z);
		if((vector.x == -1 && coordinates.x == 0) || (vector.x == 1 && coordinates.x == m_sizeX - 1))
			return BlockIndex::null();
		if((vector.y == -1 && coordinates.y == 0) || (vector.y == 1 && coordinates.y == m_sizeY - 1))
			return BlockIndex::null();
		if((vector.z == -1 && coordinates.z == 0) || (vector.z == 1 && coordinates.z == m_sizeZ - 1))
			return BlockIndex::null();
	}
	return index + m_offsetsForAdjacentCountTable[adjacentCount.get()];
}
std::array<int, 26> Blocks::makeOffsetsForAdjacentCountTable() const
{
	std::array<int, 26> output;
	uint i = 0;
	int sy = m_sizeY.get();
	int sx = m_sizeX.get();
	for(auto [x, y, z] : offsetsListAllAdjacent)
		output[i++] = (z * sy * sx) + (y * sx) + x;
	return output;
}
std::array<int32_t, 3> Blocks::relativeOffsetTo(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return {
		(int)(otherCoordinates.x - coordinates.x).get(),
		(int)(otherCoordinates.y - coordinates.y).get(),
		(int)(otherCoordinates.z - coordinates.z).get()
	};
}
bool Blocks::canSeeThrough(const BlockIndex& index) const
{
	if(solid_is(index) && !MaterialType::getTransparent(solid_get(index)))
		return false;
	const BlockFeature* door = blockFeature_atConst(index, BlockFeatureType::door);
	if(door != nullptr && !MaterialType::getTransparent(door->materialType) && door->closed)
		return false;
	return true;
}
bool Blocks::canSeeThroughFloor(const BlockIndex& index) const
{
	const BlockFeature* floor = blockFeature_atConst(index, BlockFeatureType::floor);
	if(floor != nullptr && !MaterialType::getTransparent(floor->materialType))
		return false;
	const BlockFeature* hatch = blockFeature_atConst(index, BlockFeatureType::hatch);
	if(hatch != nullptr && !MaterialType::getTransparent(hatch->materialType) && hatch->closed)
		return false;
	return true;
}
bool Blocks::canSeeThroughFrom(const BlockIndex& index, const BlockIndex& otherIndex) const
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
Facing Blocks::facingToSetWhenEnteringFrom(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	if(otherCoordinates.x > coordinates.x)
		return Facing::create(3);
	if(otherCoordinates.x < coordinates.x)
		return Facing::create(1);
	if(otherCoordinates.y < coordinates.y)
		return Facing::create(2);
	return Facing::create(0);
}
Facing Blocks::facingToSetWhenEnteringFromIncludingDiagonal(const BlockIndex& index, const BlockIndex& otherIndex, const Facing inital) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	if(coordinates.x == otherCoordinates.x)
	{
		if(coordinates.y < otherCoordinates.y)
			return Facing::create(0); // North
		if(coordinates.y == otherCoordinates.y)
			return inital; // Up or Down
		if(coordinates.y > otherCoordinates.y)
			return Facing::create(4); // South
	}
	else if(coordinates.x < otherCoordinates.x)
	{
		if(coordinates.y > otherCoordinates.y)
			return Facing::create(7); // North West
		if(coordinates.y == otherCoordinates.y)
			return Facing::create(6); // West
		if(coordinates.y < otherCoordinates.y)
			return Facing::create(5);// South West
	}
	assert(coordinates.x > otherCoordinates.x);
	if(coordinates.y > otherCoordinates.y)
		return Facing::create(3); // South East
	if(coordinates.y == otherCoordinates.y)
		return Facing::create(2); // East
	if(coordinates.y < otherCoordinates.y)
		return Facing::create(1);// North East
	assert(false);
	return Facing::null();
}
bool Blocks::isSupport(const BlockIndex& index) const
{
	return solid_is(index) || blockFeature_isSupport(index);
}
bool Blocks::isOutdoors(const BlockIndex& index) const
{
	return m_outdoors[index];
}
bool Blocks::isUnderground(const BlockIndex& index) const
{
	return m_underground[index];
}
bool Blocks::isExposedToSky(const BlockIndex& index) const
{
	return m_exposedToSky[index];
}
bool Blocks::isEdge(const BlockIndex& index) const
{
	return m_isEdge[index];
}
bool Blocks::hasLineOfSightTo(const BlockIndex& index, const BlockIndex& other) const
{
	return m_area.m_opacityFacade.hasLineOfSight(index, other);
}
BlockIndices Blocks::collectAdjacentsInRange(const BlockIndex& index, const DistanceInBlocks& range)
{
	auto condition = [&](const BlockIndex& b){ return taxiDistance(b, index) <= range; };
	return collectAdjacentsWithCondition(index, condition);
}
