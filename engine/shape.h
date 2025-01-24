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
	std::wstring name;
	uint32_t displayScale;
};

struct Shape
{
	StrongVector<std::array<std::vector<std::array<int32_t, 4>>,4>, ShapeId> m_occupiedOffsetsCache;
	StrongVector<std::array<std::vector<std::array<int32_t, 3>>,4>, ShapeId> m_adjacentOffsetsCache;
	StrongVector<std::vector<std::array<int32_t, 4>>, ShapeId> m_positions;
	StrongVector<std::wstring, ShapeId> m_name;
	StrongBitSet<ShapeId> m_isMultiTile;
	StrongBitSet<ShapeId> m_isRadiallySymetrical;
	//TODO: This doesn't belong here. Move to UI.
	StrongVector<uint32_t, ShapeId> m_displayScale;
public:
	static ShapeId create(const std::wstring name, std::vector<std::array<int32_t, 4>> positions, uint32_t displayScale);
	[[nodiscard]] static Json toJson(const ShapeId& id);
	[[nodiscard]] static std::vector<std::array<int32_t, 4>> positionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 3>> adjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 4>> makeOccupiedPositionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 3>> makeAdjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static std::vector<std::array<int32_t, 3>> positionOffsets(std::array<int32_t, 4> position);
	[[nodiscard]] static BlockIndices getBlocksOccupiedAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static BlockIndices getBlocksOccupiedAndAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static std::vector<std::pair<BlockIndex, CollisionVolume>> getBlocksOccupiedAtWithVolumes(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static BlockIndices getBlocksWhichWouldBeAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static BlockIndex getBlockWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)> predicate);
	[[nodiscard]] static BlockIndex getBlockWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)> predicate);
	[[nodiscard]] static CollisionVolume getCollisionVolumeAtLocationBlock(const ShapeId& id);
	[[nodiscard]] static std::vector<std::array<int32_t, 4>> getPositions(const ShapeId& id);
	[[nodiscard]] static std::wstring getName(const ShapeId& id);
	[[nodiscard]] static uint32_t getDisplayScale(const ShapeId& id);
	[[nodiscard]] static bool getIsMultiTile(const ShapeId& id);
	[[nodiscard]] static bool getIsRadiallySymetrical(const ShapeId& id);
	// If provided name is not found it is decoded into a custom shape.
	[[nodiscard]] static ShapeId byName(const std::wstring& name);
	[[nodiscard]] static bool hasShape(const std::wstring& name);
	// Creates a copy, adds a position to it and returns it.
	[[nodiscard]] static ShapeId mutateAdd(const ShapeId& shape, std::array<int32_t, 4> position);
	[[nodiscard]] static std::wstring makeName(std::vector<std::array<int32_t, 4>>& positions);
	[[nodiscard]] static ShapeId loadFromName(std::wstring name);
};
inline Shape shapeData;
