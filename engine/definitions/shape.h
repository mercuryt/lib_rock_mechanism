/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include "../config.h"
#include "../dataStructures/strongVector.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/mapWithCuboidKeys.h"

#include <cassert>
#include <vector>
#include <array>
#include <string>

class Space;
struct DeserializationMemo;

struct ShapeParamaters
{
	MapWithOffsetCuboidKeys<CollisionVolume> positions;
	std::string name;
	uint32_t displayScale;
};

struct Shape
{
	StrongVector<std::array<MapWithOffsetCuboidKeys<CollisionVolume>,4>, ShapeId> m_occupiedOffsetsCache;
	StrongVector<std::array<OffsetCuboid,4>, ShapeId> m_boundryOffsetCache;
	StrongVector<std::array<OffsetCuboidSet,4>, ShapeId> m_adjacentOffsetsCache;
	StrongVector<MapWithOffsetCuboidKeys<CollisionVolume>, ShapeId> m_positions;
	StrongVector<std::string, ShapeId> m_name;
	StrongBitSet<ShapeId> m_isMultiTile;
	StrongBitSet<ShapeId> m_isRadiallySymetrical;
	//TODO: This doesn't belong here. Move to UI.
	StrongVector<uint32_t, ShapeId> m_displayScale;
public:
	static ShapeId create(const std::string name, MapWithOffsetCuboidKeys<CollisionVolume>&& positions, uint32_t displayScale);
	[[nodiscard]] static Json toJson(const ShapeId& id);
	[[nodiscard]] static uint16_t size(const ShapeId& id);
	[[nodiscard]] static const MapWithOffsetCuboidKeys<CollisionVolume>& positionsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static const OffsetCuboidSet& adjacentCuboidsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static MapWithOffsetCuboidKeys<CollisionVolume> makeOccupiedCuboidsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static OffsetCuboidSet makeAdjacentCuboidsWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static OffsetCuboid makeOffsetCuboidBoundryWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static MapWithOffsetCuboidKeys<CollisionVolume> getCuboidsWithVolumeByZLevel(const ShapeId& id, const Distance& z);
	[[nodiscard]] static CuboidSet getCuboidsOccupiedAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing);
	[[nodiscard]] static CuboidSet getCuboidsOccupiedAndAdjacentAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing);
	[[nodiscard]] static MapWithCuboidKeys<CollisionVolume> getCuboidsOccupiedAtWithVolume(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing);
	[[nodiscard]] static CuboidSet getCuboidsWhichWouldBeAdjacentAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing);
	[[nodiscard]] static Point3D getPointWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)> predicate);
	[[nodiscard]] static Point3D getPointWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)> predicate);
	[[nodiscard]] static CollisionVolume getCollisionVolumeAtLocation(const ShapeId& id);
	[[nodiscard]] static CollisionVolume getTotalCollisionVolume(const ShapeId& id);
	[[nodiscard]] static uint16_t getCuboidsCount(const ShapeId& id);
	[[nodiscard]] static const MapWithOffsetCuboidKeys<CollisionVolume>& getOffsetCuboidsWithVolume(const ShapeId& id);
	[[nodiscard]] static const OffsetCuboid& getOffsetCuboidBoundryWithFacing(const ShapeId& id, const Facing4& facing);
	[[nodiscard]] static std::string getName(const ShapeId& id);
	[[nodiscard]] static uint32_t getDisplayScale(const ShapeId& id);
	[[nodiscard]] static bool getIsMultiPoint(const ShapeId& id);
	[[nodiscard]] static bool getIsRadiallySymetrical(const ShapeId& id);
	[[nodiscard]] static Offset getZSize(const ShapeId& id);
	[[nodiscard]] static Quantity getNumberOfPointsOnLeadingFaceAtOrBelowLevel(const ShapeId& id, const Distance& zLevel);
	// If provided name is not found it is decoded into a custom shape.
	[[nodiscard]] static ShapeId byName(const std::string& name);
	[[nodiscard]] static bool hasShape(const std::string& name);
	// Creates a copy, adds a position to it and returns it.
	[[nodiscard]] static ShapeId mutateAdd(const ShapeId& id, const std::pair<OffsetCuboid, CollisionVolume>& cuboidAndVolume);
	[[nodiscard]] static ShapeId mutateAddMultiple(const ShapeId& id, const MapWithOffsetCuboidKeys<CollisionVolume>& cuboidsWithVolume);
	[[nodiscard]] static ShapeId mutateRemove(const ShapeId& id, const std::pair<OffsetCuboid, CollisionVolume>& cuboid);
	[[nodiscard]] static ShapeId mutateRemoveMultiple(const ShapeId& id, const MapWithOffsetCuboidKeys<CollisionVolume>& cuboidsWithVolume);
	[[nodiscard]] static ShapeId mutateMultiplyVolume(const ShapeId& id, const Quantity& quantity);
	[[nodiscard]] static std::string makeName(const MapWithOffsetCuboidKeys<CollisionVolume>& positions);
	[[nodiscard]] static ShapeId loadFromName(std::string name);
	[[nodiscard]] static ShapeId createCustom(MapWithOffsetCuboidKeys<CollisionVolume>&& positions);
};
inline Shape g_shapeData;
