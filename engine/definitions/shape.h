/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include "config.h"
#include "dataStructures/strongVector.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "geometry/point3D.h"

#include <cassert>
#include <vector>
#include <array>
#include <string>

class Blocks;
struct DeserializationMemo;

struct OffsetAndVolume
{
	Offset3D offset;
	CollisionVolume volume;
	[[nodiscard]] bool operator==(const OffsetAndVolume& other) const { return offset == other.offset; }
	[[nodiscard]] bool operator!=(const OffsetAndVolume& other) const { return offset != other.offset; }
	[[nodiscard]] std::strong_ordering operator<=>(const OffsetAndVolume& other) const { return offset <=> other.offset; }
};
inline void to_json(Json& data, const OffsetAndVolume& x) { data[0] = x.offset; data[1] = x.volume; }
inline void from_json(const Json& data, OffsetAndVolume& x) { data[0].get_to(x.offset); data[1].get_to(x.volume); }
struct ShapeParamaters
{
	SmallSet<OffsetAndVolume> positions;
	std::string name;
	uint32_t displayScale;
};

struct Shape
{
	StrongVector<std::array<SmallSet<OffsetAndVolume>,4>, ShapeId> m_occupiedOffsetsCache;
	StrongVector<std::array<SmallSet<Offset3D>,4>, ShapeId> m_adjacentOffsetsCache;
	StrongVector<SmallSet<OffsetAndVolume>, ShapeId> m_positions;
	StrongVector<std::string, ShapeId> m_name;
	StrongBitSet<ShapeId> m_isMultiTile;
	StrongBitSet<ShapeId> m_isRadiallySymetrical;
	//TODO: This doesn't belong here. Move to UI.
	StrongVector<uint32_t, ShapeId> m_displayScale;
public:
	static ShapeId create(const std::string name, SmallSet<OffsetAndVolume> positions, uint32_t displayScale);
	[[nodiscard]] static Json toJson(const ShapeId& id);
	[[nodiscard]] static SmallSet<OffsetAndVolume> positionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static SmallSet<Offset3D> adjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static SmallSet<OffsetAndVolume> makeOccupiedPositionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static SmallSet<Offset3D> makeAdjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static SmallSet<Offset3D> positionOffsets(const OffsetAndVolume& position);
	[[nodiscard]] static SmallSet<BlockIndex> getBlocksOccupiedAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static SmallSet<BlockIndex> getBlocksOccupiedAndAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static SmallSet<std::pair<BlockIndex, CollisionVolume>> getBlocksOccupiedAtWithVolumes(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static SmallSet<BlockIndex> getBlocksWhichWouldBeAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing);
	[[nodiscard]] static BlockIndex getBlockWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)> predicate);
	[[nodiscard]] static BlockIndex getBlockWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)> predicate);
	[[nodiscard]] static CollisionVolume getCollisionVolumeAtLocationBlock(const ShapeId& id);
	[[nodiscard]] static CollisionVolume getTotalCollisionVolume(const ShapeId& id);
	[[nodiscard]] static SmallSet<OffsetAndVolume> getPositions(const ShapeId& id);
	[[nodiscard]] static std::string getName(const ShapeId& id);
	[[nodiscard]] static uint32_t getDisplayScale(const ShapeId& id);
	[[nodiscard]] static bool getIsMultiTile(const ShapeId& id);
	[[nodiscard]] static bool getIsRadiallySymetrical(const ShapeId& id);
	[[nodiscard]] static DistanceInBlocks getZSize(const ShapeId& id);
	[[nodiscard]] static SmallSet<OffsetAndVolume> getPositionsByZLevel(const ShapeId& id, const DistanceInBlocks& zLevel);
	[[nodiscard]] static Quantity getNumberOfBlocksOnLeadingFaceAtOrBelowLevel(const ShapeId& id, const DistanceInBlocks& zLevel);
	// If provided name is not found it is decoded into a custom shape.
	[[nodiscard]] static ShapeId byName(const std::string& name);
	[[nodiscard]] static bool hasShape(const std::string& name);
	// Creates a copy, adds a position to it and returns it.
	[[nodiscard]] static ShapeId mutateAdd(const ShapeId& id, const OffsetAndVolume& position);
	[[nodiscard]] static ShapeId mutateAddMultiple(const ShapeId& id, const SmallSet<OffsetAndVolume>& positions);
	[[nodiscard]] static ShapeId mutateRemove(const ShapeId& id, const OffsetAndVolume& position);
	[[nodiscard]] static ShapeId mutateRemoveMultiple(const ShapeId& id, SmallSet<OffsetAndVolume>& positions);
	[[nodiscard]] static ShapeId mutateMultiplyVolume(const ShapeId& id, const Quantity& quantity);
	[[nodiscard]] static std::string makeName(SmallSet<OffsetAndVolume>& positions);
	[[nodiscard]] static ShapeId loadFromName(std::string name);
	[[nodiscard]] static ShapeId createCustom(SmallSet<OffsetAndVolume>& positions);
};
inline Shape shapeData;
