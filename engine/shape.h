/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include "config.h"
#include "dataVector.h"
#include "types.h"
#include "index.h"

#include <cassert>
#include <vector> 
#include <array> 
#include <string>

class Blocks;
struct DeserializationMemo;

struct ShapeParamaters
{
	std::vector<std::array<int32_t, 4>> positions;
	std::string name;
	uint32_t displayScale;
};

struct Shape
{
	DataVector<std::array<std::vector<std::array<int32_t, 4>>,4>, ShapeId> m_occupiedOffsetsCache;
	DataVector<std::array<std::vector<std::array<int32_t, 3>>,4>, ShapeId> m_adjacentOffsetsCache;
	DataVector<std::vector<std::array<int32_t, 4>>, ShapeId> m_positions;
	DataVector<std::string, ShapeId> m_name;
	DataBitSet<ShapeId> m_isMultiTile;
	DataBitSet<ShapeId> m_isRadiallySymetrical;
	//TODO: This doesn't belong here. Move to UI.
	DataVector<uint32_t, ShapeId> m_displayScale;
public:
	static ShapeId create(const std::string name, std::vector<std::array<int32_t, 4>> positions, uint32_t displayScale);
	[[nodiscard]] static Json toJson(ShapeId id);
	[[nodiscard]] static std::vector<std::array<int32_t, 4>> positionsWithFacing(ShapeId id, Facing facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 3>> adjacentPositionsWithFacing(ShapeId id, Facing facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 4>> makeOccupiedPositionsWithFacing(ShapeId id, Facing facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 3>> makeAdjacentPositionsWithFacing(ShapeId id, Facing facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 3>> positionOffsets(std::array<int32_t, 4> position);
	[[nodiscard]] static BlockIndices getBlocksOccupiedAt(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing);
	[[nodiscard]] static BlockIndices getBlocksOccupiedAndAdjacentAt(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing);
	[[nodiscard]] static std::vector<std::pair<BlockIndex, CollisionVolume>> getBlocksOccupiedAtWithVolumes(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing);
	[[nodiscard]] static BlockIndices getBlocksWhichWouldBeAdjacentAt(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing);
	[[nodiscard]] static BlockIndex getBlockWhichWouldBeOccupiedAtWithPredicate(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing, std::function<bool(BlockIndex)> predicate);
	[[nodiscard]] static BlockIndex getBlockWhichWouldBeAdjacentAtWithPredicate(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing, std::function<bool(BlockIndex)> predicate);
	[[nodiscard]] static CollisionVolume getCollisionVolumeAtLocationBlock(ShapeId id);
	[[nodiscard]] static std::vector<std::array<int32_t, 4>> getPositions(ShapeId id);
	[[nodiscard]] static std::string getName(ShapeId id);
	[[nodiscard]] static uint32_t getDisplayScale(ShapeId id);
	[[nodiscard]] static bool getIsMultiTile(ShapeId id);
	[[nodiscard]] static bool getIsRadiallySymetrical(ShapeId id);
	[[nodiscard]] static ShapeId byName(const std::string& name);
	[[nodiscard]] static bool hasShape(const std::string& name);
	// creates a copy, adds a position to it and returns it.
	[[nodiscard]] static ShapeId mutateAdd(ShapeId shape, std::array<int32_t, 4> position);
	[[nodiscard]] static std::string makeName(std::vector<std::array<int32_t, 4>>& positions);
	[[nodiscard]] static ShapeId loadFromName(std::string name);
};
inline Shape shapeData;
