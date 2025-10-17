#include "shape.h"
#include "space/space.h"
#include "geometry/cuboidSet.h"
#include "deserializationMemo.h"
#include "numericTypes/types.h"
#include "util.h"
#include "geometry/point3D.h"
#include "space/adjacentOffsets.h"
#include <string>
#include <sys/types.h>
//TODO: radial symetry for 2x2 and 3x3, etc.
ShapeId Shape::create(std::string name, MapWithOffsetCuboidKeys<CollisionVolume>&& positions, uint32_t displayScale)
{
	g_shapeData.m_name.add(name);
	g_shapeData.m_displayScale.add(displayScale);
	g_shapeData.m_isMultiTile.add(positions.volume() != 1);
	g_shapeData.m_isRadiallySymetrical.add(positions.size() == 1);
	g_shapeData.m_positions.add(std::move(positions));
	g_shapeData.m_occupiedOffsetsCache.add();
	g_shapeData.m_adjacentOffsetsCache.add();
	g_shapeData.m_boundryOffsetCache.add();
	ShapeId id = ShapeId::create(g_shapeData.m_name.size() - 1);
	for(Facing4 i = Facing4::North; i != Facing4::Null; i = Facing4((uint)i + 1))
	{
		g_shapeData.m_occupiedOffsetsCache[id][(uint)i] = makeOccupiedCuboidsWithFacing(id, i);
		g_shapeData.m_adjacentOffsetsCache[id][(uint)i] = makeAdjacentCuboidsWithFacing(id, i);
		g_shapeData.m_boundryOffsetCache[id][(uint)i] = makeOffsetCuboidBoundryWithFacing(id, i);
	}
	return id;
}
const MapWithOffsetCuboidKeys<CollisionVolume>& Shape::positionsWithFacing(const ShapeId& id, const Facing4& facing) { return g_shapeData.m_occupiedOffsetsCache[id][(uint)facing]; }
const OffsetCuboidSet& Shape::adjacentCuboidsWithFacing(const ShapeId& id, const Facing4& facing) { return g_shapeData.m_adjacentOffsetsCache[id][(uint)facing]; }
MapWithOffsetCuboidKeys<CollisionVolume> Shape::makeOccupiedCuboidsWithFacing(const ShapeId& id, const Facing4& facing)
{
	//TODO: cache.
	MapWithOffsetCuboidKeys<CollisionVolume> output;
	// Make copy to rotate.
	auto shapeDataPositions = g_shapeData.m_positions[id];
	switch(facing)
	{
		case Facing4::North:
			return g_shapeData.m_positions[id];
		case Facing4::East:
			for(auto& [offsetCuboid, volume] : shapeDataPositions)
			{
				std::swap(offsetCuboid.m_high.data[0], offsetCuboid.m_high.data[1]);
				std::swap(offsetCuboid.m_low.data[0], offsetCuboid.m_low.data[1]);
				offsetCuboid = OffsetCuboid::create(offsetCuboid.m_high, offsetCuboid.m_low);
				output.insert(offsetCuboid, volume);
			}
			return output;
		case Facing4::South:
			for(auto& [offsetCuboid, volume] : shapeDataPositions)
			{
				offsetCuboid.m_high.data[1] *= -1;
				offsetCuboid.m_low.data[1] *= -1;
				offsetCuboid = OffsetCuboid::create(offsetCuboid.m_high, offsetCuboid.m_low);
				output.insert(offsetCuboid, volume);
			}
			return output;
		case Facing4::West:
			for(auto& [offsetCuboid, volume] : shapeDataPositions)
			{
				std::swap(offsetCuboid.m_high.data[0], offsetCuboid.m_high.data[1]);
				std::swap(offsetCuboid.m_low.data[0], offsetCuboid.m_low.data[1]);
				offsetCuboid.m_high.data[0] *= -1;
				offsetCuboid.m_low.data[0] *= -1;
				offsetCuboid = OffsetCuboid::create(offsetCuboid.m_high, offsetCuboid.m_low);
				output.insert(offsetCuboid, volume);
			}
			return output;
		default:
			std::unreachable();
	}
	std::unreachable();
}
OffsetCuboidSet Shape::makeAdjacentCuboidsWithFacing(const ShapeId& id, const Facing4& facing)
{
	OffsetCuboidSet output;
	const auto& occupiedOffsets = g_shapeData.m_occupiedOffsetsCache[id][(uint)facing];
	for(const auto& [offsetCuboid, volume] : occupiedOffsets)
	{
		OffsetCuboid inflated = offsetCuboid;
		inflated.inflate({1});
		output.add(inflated);
	}
	// Leave the occupied cuboids in the set for simple shapes.
	if(occupiedOffsets.size() > 4)
		for(const auto& [offsetCuboid, volume] : occupiedOffsets)
			output.removeContainedAndFragmentIntercepted(offsetCuboid);
	return output;
}
MapWithOffsetCuboidKeys<CollisionVolume> Shape::getCuboidsWithVolumeByZLevel(const ShapeId& id, const Distance& z)
{
	MapWithOffsetCuboidKeys<CollisionVolume> output;
	const OffsetCuboid plane{Offset3D(Offset::max(), Offset::max(), Offset::create(z.get())), Offset3D::create(0, 0, z.get())};
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_positions[id])
		if(plane.intersects(offsetCuboid))
			output.insertOrMerge(plane.intersection(offsetCuboid), volume);
	return output;
}
CuboidSet Shape::getCuboidsOccupiedAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	CuboidSet output;
	output.reserve(g_shapeData.m_positions[id].size());
	const OffsetCuboid offsetBoundry = space.offsetBoundry();
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		const OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(location);
		assert(offsetBoundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid  = Cuboid::create(relativeOffsetCuboid);
		output.add(cuboid);
	}
	return output;
}
OffsetCuboid Shape::makeOffsetCuboidBoundryWithFacing(const ShapeId& id, const Facing4& facing)
{
	OffsetCuboid output;
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_occupiedOffsetsCache[id][(uint)facing])
		if(!output.exists())
			output = offsetCuboid;
		else
			output.maybeExpand(offsetCuboid);
	return output;
}
CuboidSet Shape::getCuboidsOccupiedAndAdjacentAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	CuboidSet output;
	const OffsetCuboid offsetBoundry = space.offsetBoundry();
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(location);
		relativeOffsetCuboid.inflate({1});
		const Cuboid cuboid = Cuboid::create(offsetBoundry.intersection(relativeOffsetCuboid));
		output.add(cuboid);
	}
	return output;
}
MapWithCuboidKeys<CollisionVolume> Shape::getCuboidsOccupiedAtWithVolume(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	assert(location.exists());
	assert(facing != Facing4::Null);
	MapWithCuboidKeys<CollisionVolume> output;
	output.reserve(g_shapeData.m_positions[id].size());
	const OffsetCuboid offsetBoundry = space.offsetBoundry();
	for(const auto& [offsetCuboid, volume] : positionsWithFacing(id, facing))
	{
		const OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(location);
		assert(offsetBoundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid = Cuboid::create(relativeOffsetCuboid);
		output.insert(cuboid, volume);
	}
	return output;
}
CuboidSet Shape::getCuboidsWhichWouldBeAdjacentAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	CuboidSet output;
	output.reserve(g_shapeData.m_positions[id].size());
	const OffsetCuboid offsetBoundry = space.offsetBoundry();
	for(const OffsetCuboid& offsetCuboid : g_shapeData.m_adjacentOffsetsCache[id][(uint)facing])
	{
		OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(location);
		if(!offsetBoundry.intersects(relativeOffsetCuboid))
			continue;
		const Cuboid cuboid = Cuboid::create(offsetBoundry.intersection(relativeOffsetCuboid));
		output.add(cuboid);
	}
	return output;
}
Point3D Shape::getPointWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)> predicate)
{
	const OffsetCuboid offsetBoundry = space.offsetBoundry();
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(location);
		assert(offsetBoundry.contains(relativeOffsetCuboid));
		const Cuboid cuboid = Cuboid::create(relativeOffsetCuboid);
		for(const Point3D& point : cuboid)
			if(predicate(point))
				return point;
	}
	return Point3D::null();
}
Point3D Shape::getPointWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)> predicate)
{
	const OffsetCuboid offsetBoundry = space.offsetBoundry();
	for(const OffsetCuboid& offsetCuboid : g_shapeData.m_adjacentOffsetsCache[id][(uint)facing])
	{
		OffsetCuboid relativeOffsetCuboid = offsetCuboid.relativeToPoint(location);
		if(!offsetBoundry.intersects(relativeOffsetCuboid))
			continue;
		const Cuboid cuboid = Cuboid::create(offsetBoundry.intersection(relativeOffsetCuboid));
		for(const Point3D& point : cuboid)
			if(predicate(point))
				return point;
	}
	return Point3D::null();
}
CollisionVolume Shape::getCollisionVolumeAtLocation(const ShapeId& id)
{
	const auto& [offsetCuboid, volume] = g_shapeData.m_positions[id].front();
	assert(offsetCuboid.contains(Offset3D::create(0,0,0)));
	return volume;
}
CollisionVolume Shape::getTotalCollisionVolume(const ShapeId& id)
{
	CollisionVolume output = CollisionVolume::create(0);
	for(const auto& [offset, volume] : getOffsetCuboidsWithVolume(id))
		output += volume;
	return output;
}
uint16_t Shape::getCuboidsCount(const ShapeId& id)
{
	return getOffsetCuboidsWithVolume(id).size();
}
const MapWithOffsetCuboidKeys<CollisionVolume>& Shape::getOffsetCuboidsWithVolume(const ShapeId& id)
{
	return g_shapeData.m_positions[id];
}
const OffsetCuboid& Shape::getOffsetCuboidBoundryWithFacing(const ShapeId& id, const Facing4& facing)
{
	return g_shapeData.m_boundryOffsetCache[id][(int)facing];
}
ShapeId Shape::byName(const std::string& name)
{
	auto found = g_shapeData.m_name.find(name);
	if(found == g_shapeData.m_name.end())
		return loadFromName(name);
	return ShapeId::create(found - g_shapeData.m_name.begin());
}
std::string Shape::getName(const ShapeId& id) { return g_shapeData.m_name[id]; }
uint32_t Shape::getDisplayScale(const ShapeId& id) { return g_shapeData.m_displayScale[id]; }
bool Shape::getIsMultiTile(const ShapeId& id) { return g_shapeData.m_isMultiTile[id]; }
bool Shape::getIsRadiallySymetrical(const ShapeId& id) { return g_shapeData.m_isRadiallySymetrical[id]; }
// TODO: cache this.
Offset Shape::getZSize(const ShapeId& id)
{
	assert(!g_shapeData.m_positions[id].empty());
	//TODO: why aren't the type of high and low OffsetWidth?
	Offset high = Offset::min();
	Offset low = Offset::max();
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_positions[id])
	{
		high = std::max(high, offsetCuboid.m_high.z());
		low = std::min(low, offsetCuboid.m_low.z());
	}
	return high - low;
}
Quantity Shape::getNumberOfPointsOnLeadingFaceAtOrBelowLevel(const ShapeId& id, const Distance& zLevel)
{
	Quantity output = {0};
	Offset zLevelOffset = Offset::create(zLevel.get());
	for(const auto& [offsetCuboid, volume] : g_shapeData.m_positions[id])
	{
		assert(offsetCuboid.m_high.z() >= 0);
		Offset offsetCuboidHighZ = offsetCuboid.m_high.z();
		if(zLevelOffset <= offsetCuboidHighZ)
		{
			const OffsetCuboid leadingFace = offsetCuboid.getFace(Facing6::North);
			Offset faceAreaPerZLevel = leadingFace.m_high.y() - leadingFace.m_low.y();
			Offset distanceZ = std::min(zLevelOffset, offsetCuboidHighZ) - offsetCuboid.m_low.z();
			output += (faceAreaPerZLevel * distanceZ).get();
		}
	}
	return output;
}
bool Shape::hasShape(const std::string& name)
{
	auto found = g_shapeData.m_name.find(name);
	return found != g_shapeData.m_name.end();
}
MapWithOffsetCuboidKeys<CollisionVolume> Shape::applyOffsetAndRotationAndSubtractOriginal(const ShapeId& shape, const Offset3D& offset, const Facing4& initalFacing, const Facing4& newFacing)
{
	const MapWithOffsetCuboidKeys<CollisionVolume>& positionsWithInitalFacing = g_shapeData.m_occupiedOffsetsCache[shape][(uint)initalFacing];
	const MapWithOffsetCuboidKeys<CollisionVolume>& positionsWithNewFacing = g_shapeData.m_occupiedOffsetsCache[shape][(uint)newFacing];
	MapWithOffsetCuboidKeys<CollisionVolume> positionsWithNewFacingOffset = positionsWithNewFacing.applyOffset(offset);
	// TODO: Make difference in volumes instead of treating the inital facing positions as full volume?
	positionsWithNewFacingOffset.removeContainedAndFragmentInterceptedAll(positionsWithInitalFacing.makeCuboidSet());
	return positionsWithNewFacingOffset;
}
ShapeId Shape::loadFromName(std::string name)
{
	Json data = Json::parse(name);
	MapWithOffsetCuboidKeys<CollisionVolume> positions;
	data.get_to(positions);
	// Runtime shapes always have display scale = 1
	return Shape::create(name, std::move(positions), 1);
}
ShapeId Shape::mutateAdd(const ShapeId& id, const std::pair<OffsetCuboid, CollisionVolume>& position)
{
	MapWithOffsetCuboidKeys<CollisionVolume> positions;
	positions.insert(position);
	return mutateAddMultiple(id, positions);
}
ShapeId Shape::mutateAddMultiple(const ShapeId& shape, const MapWithOffsetCuboidKeys<CollisionVolume>& positions)
{
	// Make a copy.
	MapWithOffsetCuboidKeys<CollisionVolume> merged = g_shapeData.m_positions[shape];
	merged.insertAll(positions);
	// TODO: merge cuboids.
	return createCustom(std::move(merged));
}
ShapeId Shape::mutateRemove(const ShapeId& id, const std::pair<OffsetCuboid, CollisionVolume>& pair)
{
	MapWithOffsetCuboidKeys<CollisionVolume> cuboids;
	cuboids.insert(pair);
	return mutateRemoveMultiple(id, cuboids);
}
ShapeId Shape::mutateRemoveMultiple(const ShapeId& id, const MapWithOffsetCuboidKeys<CollisionVolume>& cuboids)
{
	// Make a copy.
	MapWithOffsetCuboidKeys<CollisionVolume> purged;
	for(const auto& [toRemove, volumeToRemove] : cuboids)
	{
		for(const auto& [offsetCuboid, volume] : g_shapeData.m_positions[id])
			// TODO: use volume to remove in order to make possible multiple small creatures riding at the same location.
			if(toRemove.intersects(offsetCuboid))
			{
				if(toRemove.contains(offsetCuboid))
					continue;
				for(const OffsetCuboid& fragment : offsetCuboid.getChildrenWhenSplitByCuboid(toRemove))
					purged.insert(fragment, volume);
			}
			else
				purged.insert({offsetCuboid, volume});
	}
	purged.sort();
	return createCustom(std::move(purged));
}
ShapeId Shape::mutateMultiplyVolume(const ShapeId& id, const Quantity& quantity)
{
	// Make a copy.
	MapWithOffsetCuboidKeys<CollisionVolume> copy = g_shapeData.m_positions[id];
	for(auto& [offsetCuboid, volume]  : copy)
		volume *= quantity.get();
	return createCustom(std::move(copy));
}
std::string Shape::makeName(const MapWithOffsetCuboidKeys<CollisionVolume>& positions)
{
	Json data(positions);
	return data.dump();
}
ShapeId Shape::createCustom(MapWithOffsetCuboidKeys<CollisionVolume>&& positions)
{
	positions.sort();
	std::string name = makeName(positions);
	ShapeId output = Shape::byName(name);
	if(output.exists())
		return output;
	// Runtime shapes always have display scale = 1
	return Shape::create(name, std::move(positions), 1);
}