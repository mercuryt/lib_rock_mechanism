/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include "../config/config.h"
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
	int displayScale;
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
	StrongVector<int, ShapeId> m_displayScale;
public:
	static ShapeId create(const std::string name, MapWithOffsetCuboidKeys<CollisionVolume>&& positions, int displayScale);
	[[nodiscard]] static Json toJson(ShapeId id);
	[[nodiscard]] static int size(ShapeId id);
	[[nodiscard]] static OffsetCuboid getBoundry(ShapeId id, Facing4 facing = Facing4::North);
	[[nodiscard]] static const MapWithOffsetCuboidKeys<CollisionVolume>& positionsWithFacing(ShapeId id, const Facing4 facing);
	[[nodiscard]] static const OffsetCuboidSet& adjacentCuboidsWithFacing(ShapeId id, const Facing4 facing);
	[[nodiscard]] static MapWithOffsetCuboidKeys<CollisionVolume> makeOccupiedCuboidsWithFacing(ShapeId id, const Facing4 facing);
	[[nodiscard]] static OffsetCuboidSet makeAdjacentCuboidsWithFacing(ShapeId id, const Facing4 facing);
	[[nodiscard]] static OffsetCuboid makeOffsetCuboidBoundryWithFacing(ShapeId id, const Facing4 facing);
	[[nodiscard]] static MapWithOffsetCuboidKeys<CollisionVolume> getCuboidsWithVolumeByZLevel(ShapeId id, const Distance z);
	[[nodiscard]] static CuboidSet getCuboidsOccupiedAt(ShapeId id, const Space& space, const Point3D location, const Facing4 facing);
	[[nodiscard]] static CuboidSet getCuboidsOccupiedAndAdjacentAt(ShapeId id, const Space& space, const Point3D location, const Facing4 facing);
	[[nodiscard]] static MapWithCuboidKeys<CollisionVolume> getCuboidsOccupiedAtWithVolume(ShapeId id, const Space& space, const Point3D location, const Facing4 facing);
	[[nodiscard]] static CuboidSet getCuboidsWhichWouldBeAdjacentAt(ShapeId id, const Space& space, const Point3D location, const Facing4 facing);
	[[nodiscard]] static Point3D getPointWhichWouldBeOccupiedAtWithPredicate(ShapeId id, const Space& space, const Point3D location, const Facing4 facing, std::function<bool(const Point3D)> predicate);
	[[nodiscard]] static Point3D getPointWhichWouldBeAdjacentAtWithPredicate(ShapeId id, const Space& space, const Point3D location, const Facing4 facing, std::function<bool(const Point3D)> predicate);
	[[nodiscard]] static CollisionVolume getCollisionVolumeAtLocation(ShapeId id);
	[[nodiscard]] static CollisionVolume getTotalCollisionVolume(ShapeId id);
	[[nodiscard]] static int getCuboidsCount(ShapeId id);
	[[nodiscard]] static const MapWithOffsetCuboidKeys<CollisionVolume>& getOffsetCuboidsWithVolume(ShapeId id);
	[[nodiscard]] static const OffsetCuboid getOffsetCuboidBoundryWithFacing(ShapeId id, const Facing4 facing);
	[[nodiscard]] static const Cuboid getBoundryAtWithFacing(ShapeId id, const Space& space, const Point3D location, const Facing4 facing);
	[[nodiscard]] static std::string getName(ShapeId id);
	[[nodiscard]] static int getDisplayScale(ShapeId id);
	[[nodiscard]] static bool getIsMultiTile(ShapeId id);
	[[nodiscard]] static bool getIsRadiallySymetrical(ShapeId id);
	[[nodiscard]] static Offset getZSize(ShapeId id);
	[[nodiscard]] static Distance getDistanceFromBack(ShapeId id);
	[[nodiscard]] static Distance getDistanceFromFront(ShapeId id);
	[[nodiscard]] static Distance getHeight(ShapeId id);
	[[nodiscard]] static Quantity getNumberOfPointsOnLeadingFaceAtOrBelowLevel(ShapeId id, const Distance zLevel);
	[[nodiscard]] static bool isInBoundsAt(ShapeId id, const Space& space, Point3D location, Facing4 facing);
	// If provided name is not found it is decoded into a custom shape.
	[[nodiscard]] static ShapeId byName(const std::string& name);
	[[nodiscard]] static bool hasShape(const std::string& name);
	[[nodiscard]] static MapWithOffsetCuboidKeys<CollisionVolume> applyOffsetAndRotationAndSubtractOriginal(ShapeId shape, const Offset3D offset, const Facing4 initialFacing, const Facing4 newFacing);
	// Creates a copy, adds a position to it and returns it.
	[[nodiscard]] static ShapeId mutateAdd(ShapeId id, const std::pair<OffsetCuboid, CollisionVolume>& cuboidAndVolume);
	[[nodiscard]] static ShapeId mutateAddMultiple(ShapeId id, const MapWithOffsetCuboidKeys<CollisionVolume>& cuboidsWithVolume);
	[[nodiscard]] static ShapeId mutateRemove(ShapeId id, const std::pair<OffsetCuboid, CollisionVolume>& cuboid);
	[[nodiscard]] static ShapeId mutateRemoveMultiple(ShapeId id, const MapWithOffsetCuboidKeys<CollisionVolume>& cuboidsWithVolume);
	[[nodiscard]] static ShapeId mutateMultiplyVolume(ShapeId id, const Quantity quantity);
	[[nodiscard]] static Distance getRadius(ShapeId id);
	[[nodiscard]] static std::string makeName(const MapWithOffsetCuboidKeys<CollisionVolume>& positions);
	[[nodiscard]] static ShapeId loadFromName(std::string name);
	[[nodiscard]] static ShapeId createCustom(MapWithOffsetCuboidKeys<CollisionVolume>&& positions);
};
inline Shape g_shapeData;
