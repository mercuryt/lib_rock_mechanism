#include "blocks.h"
#include "adjacentOffsets.h"
#include "../area.h"
#include "../materialType.h"
#include "../types.h"
#include "../blockFeature.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include <string>

Blocks::Blocks(Area& area, const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) :
	m_area(area),
	m_pointToIndexConversionMultipliers(1, x.get(), x.get() * y.get()),
	m_pointToIndexConversionMultipliersChunked(1, x.get() / 16, (x.get() / 16) * (y.get() / 16)),
	m_dimensions(x.get(), y.get(), z.get()),
	m_sizeXInChunks(x.get() / 16),
	m_sizeXTimesYInChunks((x.get() / 16) * ( y.get() / 16)),
	m_sizeX(x),
	m_sizeY(y),
	m_sizeZ(z),
	m_zLevelSize(x.get() * y.get())
{
	assert(!area.m_loaded);
	BlockIndex count = BlockIndex::create((x * y * z).get());
	resize(count);
	m_exposedToSky.initalizeEmpty();
	for(BlockIndex i = BlockIndex::create(0); i < count; ++i)
		initalize(i);
	makeIndexOffsetsForAdjacent();
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
	m_directlyAdjacent.resize(count);
	m_exposedToSky.resize(count);
	m_isEdge.resize(count);
	m_visible.resize(count);
	m_constructed.resize(count);
}
// Create from scratch, not load from json.
void Blocks::initalize(const BlockIndex& index)
{
	// Default initalization is fine for most things.
	Point3D point = getCoordinates(index);
	m_totalFluidVolume[index] = CollisionVolume::create(0);
	m_dynamicVolume[index] = CollisionVolume::create(0);
	m_staticVolume[index] = CollisionVolume::create(0);
	m_temperatureDelta[index] = TemperatureDelta::create(0);
	m_isEdge.set(index, ( point.data == 0 || point.data == m_dimensions - 1).any());
	recordAdjacent(index);
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
		m_mist[BlockIndex::create(std::stoi(key))] = FluidType::byName(value.get<std::wstring>());
	for(auto& [key, value] : data["mistInverseDistanceFromSource"].items())
		m_mistInverseDistanceFromSource[BlockIndex::create(std::stoi(key))] = value.get<DistanceInBlocks>();
	for(auto& [key, value] : data["reservables"].items())
	{
		auto& reservable = m_reservables[BlockIndex::create(std::stoi(key))] = std::make_unique<Reservable>(Quantity::create(1u));
		deserializationMemo.m_reservables[value.get<uintptr_t>()] = reservable.get();
	}
}
std::vector<BlockIndex> Blocks::getAllIndices() const
{
	std::vector<BlockIndex> output;
	Blocks& blocks = const_cast<Blocks&>(*this);
	Cuboid cuboid = blocks.getAll();
	for(const BlockIndex& block : cuboid.getView(blocks))
		output.push_back(block);
	return output;
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
size_t Blocks::getChunkedSize() const
{
	auto dimensionsInChunks = m_dimensions / 16;
	auto underSize = (m_dimensions - (dimensionsInChunks * 16)) != 0;
	return ((dimensionsInChunks + underSize.cast<uint>()) * 16).prod();
}
BlockIndex Blocks::getIndex(const Point3D& coordinates) const
{
	assert((coordinates.data < m_dimensions).all());
	return BlockIndex::create((coordinates.data * m_pointToIndexConversionMultipliers).sum());
}
BlockIndex Blocks::maybeGetIndex(const Point3D& coordinates) const
{
	if((coordinates.data < m_dimensions).all())
		return BlockIndex::create((coordinates.data * m_pointToIndexConversionMultipliers).sum());
	return BlockIndex::null();
}
BlockIndex Blocks::maybeGetIndexFromOffsetOnEdge(const Point3D& coordinates, const Offset3D& offset) const
{
	if(
		!(// Invert condition to put the common path in the if rather then else, as a hint to branch predictor.
			(coordinates.data == 0 && offset.data == -1).any() ||
			(coordinates.data == m_dimensions - 1 && offset.data == 1).any()
		)
	)
		return getIndex(coordinates + offset);
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
static const Coordinates localSizeMultiples(1, 16, 16*16);
BlockIndexChunked Blocks::getIndexChunked(Point3D coordinates)
{
	auto chunkOffsets = coordinates.data / 16u;
	coordinates.data -= chunkOffsets * 16;
	uint localComponent = (coordinates.data * localSizeMultiples).sum();
	uint globalComponent = (chunkOffsets * m_pointToIndexConversionMultipliersChunked).sum();
	globalComponent *= 16 * 16 * 16;
	return BlockIndexChunked::create(localComponent + globalComponent);
}
Point3D Blocks::getCoordinates(BlockIndex index) const
{
	Point3D output;
	output.data[2] = index.get() / m_zLevelSize;
	index -= BlockIndex::create((output.z() * m_zLevelSize).get());
	output.data[1] = index.get() / m_sizeX.get();
	output.data[0] = index.get() - (output.y() * m_sizeX).get();
	return output;
}
Point3D_fractional Blocks::getCoordinatesFractional(const BlockIndex& index) const
{
	return Point3D_fractional::create(getCoordinates(index));
}
DistanceInBlocks Blocks::getZ(const BlockIndex& index) const
{
	return DistanceInBlocks::create(index.get()) / (m_zLevelSize);
}
BlockIndex Blocks::getAtFacing(const BlockIndex& index, const Facing6& facing) const
{
	return m_directlyAdjacent[index][(uint)facing];
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
Cuboid Blocks::getZLevel(const DistanceInBlocks &z)
{
	return Cuboid({m_sizeX - 1, m_sizeY - 1, z}, {DistanceInBlocks::create(0), DistanceInBlocks::create(0), z});
}
BlockIndex Blocks::getMiddleAtGroundLevel() const
{
	DistanceInBlocks x = m_sizeX / 2;
	DistanceInBlocks y = m_sizeY / 2;
	const BlockIndex& middleAtTopLevel = getIndex(x, y, m_sizeZ - 1);
	DistanceInBlocks z = getZ(middleAtTopLevel);
	return getIndex(x, y, z);
}
template<uint size, bool filter>
auto getAdjacentWithOffsets(const Blocks& blocks, const BlockIndex& index, const Offset3D* offsetList) -> std::array<BlockIndex, size>
{
	std::array<BlockIndex, size> output;
	Point3D coordinates = blocks.getCoordinates(index);
	if(!blocks.isEdge(index))
		for(uint i = 0; i < size; i++)
		{
			const auto& offsets = *(offsetList + i);
			output[i] = blocks.getIndex(coordinates + offsets);
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
	m_directlyAdjacent[index] = getAdjacentWithOffsets<6, false>(*this, index, &adjacentOffsets::direct.front());
}
const std::array<BlockIndex, 6>& Blocks::getDirectlyAdjacent(const BlockIndex& index) const
{
	assert(m_directlyAdjacent.size() > index);
	return m_directlyAdjacent[index];
}
BlockIndex Blocks::getBlockBelow(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][uint(Facing6::Below)];
}
BlockIndex Blocks::getBlockAbove(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][uint(Facing6::Above)];
}
BlockIndex Blocks::getBlockNorth(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][uint(Facing6::North)];
}
BlockIndex Blocks::getBlockWest(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][uint(Facing6::West)];
}
BlockIndex Blocks::getBlockSouth(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][uint(Facing6::South)];
}
BlockIndex Blocks::getBlockEast(const BlockIndex& index) const
{
	return m_directlyAdjacent[index][uint(Facing6::East)];
}
BlockIndexSetSIMD<26> Blocks::getAdjacentWithEdgeAndCornerAdjacent(const BlockIndex& index) const
{
	if(isEdge(index))
		return m_indexOffsetsForAdjacentAll.getIndicesMaybeOutOfBounds(*this, index);
	else
		return m_indexOffsetsForAdjacentAll.getIndicesInBounds(*this, index);
}
BlockIndexSetSIMD<24> Blocks::getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(const BlockIndex& index) const
{
	if(isEdge(index))
		return m_indexOffsetsForAdjacentAllExceptDirectlyAboveAndBelow.getIndicesMaybeOutOfBounds(*this, index);
	else
		return m_indexOffsetsForAdjacentAllExceptDirectlyAboveAndBelow.getIndicesInBounds(*this, index);
}
BlockIndexSetSIMD<20> Blocks::getEdgeAndCornerAdjacentOnly(const BlockIndex& index) const
{
	if(isEdge(index))
		return m_indexOffsetsForAdjacentOnlyEdgesAndCorners.getIndicesMaybeOutOfBounds(*this, index);
	else
		return m_indexOffsetsForAdjacentOnlyEdgesAndCorners.getIndicesInBounds(*this, index);
}
BlockIndexSetSIMD<18> Blocks::getAdjacentWithEdgeAdjacent(const BlockIndex& index) const
{
	if(isEdge(index))
		return m_indexOffsetsForAdjacentDirectAndEdge.getIndicesMaybeOutOfBounds(*this, index);
	else
		return m_indexOffsetsForAdjacentDirectAndEdge.getIndicesInBounds(*this, index);
}
BlockIndexArrayNotNull<12> Blocks::getEdgeAdjacentOnly(const BlockIndex& index) const
{
	return getAdjacentWithOffsets<12, true>(*this, index, adjacentOffsets::edge.begin());
}
BlockIndexArrayNotNull<4> Blocks::getEdgeAdjacentOnSameZLevelOnly(const BlockIndex& index) const
{
	return getAdjacentWithOffsets<4, true>(*this, index, adjacentOffsets::edgeWithSameZ.begin());
}
BlockIndexArrayNotNull<4> Blocks::getAdjacentOnSameZLevelOnly(const BlockIndex& index) const
{
	return getAdjacentWithOffsets<4, true>(*this, index, adjacentOffsets::directWithSameZ.begin());
}
DistanceInBlocks Blocks::distance(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	return DistanceInBlocks::create(pow(distanceSquared(index, otherIndex).get(), 0.5));
}
DistanceInBlocks Blocks::taxiDistance(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return coordinates.taxiDistanceTo(otherCoordinates);
}
DistanceInBlocks Blocks::distanceSquared(const BlockIndex& index, const BlockIndex& other) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(other);
	return coordinates.distanceSquared(otherCoordinates);
}
DistanceInBlocksFractional Blocks::distanceFractional(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	return getCoordinates(index).distanceToFractional(getCoordinates(otherIndex));
}
bool Blocks::squareOfDistanceIsMoreThen(const BlockIndex& index, const BlockIndex& otherIndex, DistanceInBlocksFractional otherDistanceSquared) const
{
	return distanceSquared(index, otherIndex)> otherDistanceSquared.get();
}
bool Blocks::isAdjacentToAny(const BlockIndex& index, BlockIndices& blocks) const
{
	auto adjacents = getAdjacentWithEdgeAndCornerAdjacent(index);
	for(const BlockIndex& candidate : blocks)
		if(adjacents.contains(candidate))
			return true;
	return false;
}
bool Blocks::isAdjacentTo(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	// TODO: SIMD.
	for(BlockIndex adjacent : getDirectlyAdjacent(index))
		if(adjacent.exists() && otherIndex == adjacent)
			return true;
	return false;
}
bool Blocks::isAdjacentToIncludingCornersAndEdges(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	return getAdjacentWithEdgeAndCornerAdjacent(index).contains(otherIndex);
}
bool Blocks::isAdjacentToActor(const BlockIndex& index, const ActorIndex& actor) const
{
	auto adjacents = getAdjacentWithEdgeAndCornerAdjacent(index);
	for(const BlockIndex& candidate : m_area.getActors().getBlocks(actor))
		if(adjacents.contains(candidate))
			return true;
	return false;
}
bool Blocks::isAdjacentToItem(const BlockIndex& index, const ItemIndex& item) const
{
	auto adjacents = getAdjacentWithEdgeAndCornerAdjacent(index);
	for(const BlockIndex& candidate : m_area.getItems().getBlocks(item))
		if(adjacents.contains(candidate))
			return true;
	return false;
}
void Blocks::setExposedToSky(const BlockIndex& index, bool exposed)
{
	if(exposed)
		m_exposedToSky.set(m_area, index);
	else
		m_exposedToSky.unset(m_area, index);
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
BlockIndex Blocks::offset(const BlockIndex& index, int32_t ax, int32_t ay, int32_t az) const
{
	return offset(index, {ax, ay, az});
}
BlockIndex Blocks::offset(const BlockIndex& index, const Offset3D& offset) const
{
	Point3D coordinates = getCoordinates(index);
	return maybeGetIndex(coordinates + offset);
}
BlockIndex Blocks::offsetNotNull(const BlockIndex& index, int32_t ax, int32_t ay, int32_t az) const
{
	Point3D coordinates = getCoordinates(index);
	Offset3D offset(ax, ay, az);
	return getIndex(coordinates + offset);
}
BlockIndex Blocks::indexAdjacentToAtCount(const BlockIndex& index, const AdjacentIndex& adjacentCount) const
{
	if(m_isEdge[index])
		return maybeGetIndexFromOffsetOnEdge(getCoordinates(index), adjacentOffsets::all[adjacentCount.get()]);
	else
		return index + m_indexOffsetsForAdjacentAll.m_indexData[adjacentCount.get()];
}
void Blocks::makeIndexOffsetsForAdjacent()
{
	for(const Offset3D& offset : adjacentOffsets::all)
		m_indexOffsetsForAdjacentAll.insert(*this, offset);
	for(const Offset3D& offset : adjacentOffsets::allExceptDirectlyAboveAndBelow)
		m_indexOffsetsForAdjacentAllExceptDirectlyAboveAndBelow.insert(*this, offset);
	for(const Offset3D& offset : adjacentOffsets::edgeAndCorner)
		m_indexOffsetsForAdjacentOnlyEdgesAndCorners.insert(*this, offset);
	for(const Offset3D& offset : adjacentOffsets::directAndEdge)
		m_indexOffsetsForAdjacentDirectAndEdge.insert(*this, offset);
}
Offset3D Blocks::relativeOffsetTo(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return Offsets(otherCoordinates.data.cast<int>() - coordinates.data.cast<int>());
}
bool Blocks::canSeeThrough(const BlockIndex& index) const
{
	const MaterialTypeId& material = solid_get(index);
	if(material.exists() && !MaterialType::getTransparent(material))
		return false;
	const BlockFeature* door = blockFeature_atConst(index, BlockFeatureType::door);
	if(door != nullptr && door->closed && !MaterialType::getTransparent(door->materialType) )
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
Facing8 Blocks::facingToSetWhenEnteringFromIncludingDiagonal(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return otherCoordinates.getFacingTwordsIncludingDiagonal(coordinates);
}
Facing4 Blocks::facingToSetWhenEnteringFrom(const BlockIndex& index, const BlockIndex& otherIndex) const
{
	Point3D coordinates = getCoordinates(index);
	Point3D otherCoordinates = getCoordinates(otherIndex);
	return otherCoordinates.getFacingTwords(coordinates);
}
bool Blocks::isSupport(const BlockIndex& index) const
{
	return solid_is(index) || blockFeature_isSupport(index);
}
bool Blocks::isExposedToSky(const BlockIndex& index) const
{
	return m_exposedToSky.check(index);
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
std::wstring Blocks::toString(const BlockIndex& index) const
{
	return getCoordinates(index).toString();
}