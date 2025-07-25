#include "definitions/shape.h"
#include "space/space.h"
#include "geometry/cuboid.h"
#include "deserializationMemo.h"
#include "numericTypes/types.h"
#include "util.h"
#include "geometry/point3D.h"
#include "space/adjacentOffsets.h"
#include <string>
#include <sys/types.h>
//TODO: radial symetry for 2x2 and 3x3, etc.
ShapeId Shape::create(std::string name, SmallSet<OffsetAndVolume> positions, uint32_t displayScale)
{
	shapeData.m_name.add(name);
	shapeData.m_positions.add(positions);
	shapeData.m_displayScale.add(displayScale);
	shapeData.m_isMultiTile.add(positions.size() != 1);
	shapeData.m_isRadiallySymetrical.add(positions.size() == 1);
	shapeData.m_occupiedOffsetsCache.add();
	shapeData.m_adjacentOffsetsCache.add();
	ShapeId id = ShapeId::create(shapeData.m_name.size() - 1);
	for(Facing4 i = Facing4::North; i != Facing4::Null; i = Facing4((uint)i + 1))
	{
		shapeData.m_occupiedOffsetsCache[id][(uint)i] = makeOccupiedPositionsWithFacing(id, i);
		shapeData.m_adjacentOffsetsCache[id][(uint)i] = makeAdjacentPositionsWithFacing(id, i);
	}
	return id;
}
SmallSet<OffsetAndVolume> Shape::positionsWithFacing(const ShapeId& id, const Facing4& facing) { return shapeData.m_occupiedOffsetsCache[id][(uint)facing]; }
SmallSet<Offset3D> Shape::adjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing) { return shapeData.m_adjacentOffsetsCache[id][(uint)facing]; }
SmallSet<OffsetAndVolume> Shape::makeOccupiedPositionsWithFacing(const ShapeId& id, const Facing4& facing)
{
	//TODO: cache.
	SmallSet<OffsetAndVolume> output;
	switch(facing)
	{
		case Facing4::North:
			return shapeData.m_positions[id];
		case Facing4::East:
			for(auto pair : shapeData.m_positions[id])
			{
				std::swap(pair.offset.data[0], pair.offset.data[1]);
				output.insert(pair);
			}
			return output;
		case Facing4::South:
			for(auto pair : shapeData.m_positions[id])
			{
				pair.offset.data[1] *= -1;
				output.insert(pair);
			}
			return output;
		case Facing4::West:
			for(auto pair : shapeData.m_positions[id])
			{
				std::swap(pair.offset.data[0], pair.offset.data[1]);
				pair.offset.data[0] *= -1;
				output.insert(pair);
			}
			return output;
		default:
			std::unreachable();
	}
	std::unreachable();
	return output;
}
SmallSet<Offset3D> Shape::makeAdjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing)
{
	SmallSet<Offset3D> output;
	for(const auto& position : shapeData.m_occupiedOffsetsCache[id][(uint)facing])
		for(auto& offset : positionOffsets(position))
			output.maybeInsert(offset);
	return output;
}
SmallSet<Offset3D> Shape::positionOffsets(const OffsetAndVolume& offsetAndVolume)
{
	SmallSet<Offset3D> output;
	for(const auto& offsets : adjacentOffsets::all)
		output.insert(offsetAndVolume.offset + offsets);
	return output;
}

SmallSet<Point3D> Shape::getPointsOccupiedAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	SmallSet<Point3D> output;
	output.reserve(shapeData.m_positions[id].size());
	for(const auto& pair : shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		Point3D point = location.applyOffset(pair.offset);
		assert(space.getAll().contains(point));
		output.insert(point);
	}
	return output;
}
SmallSet<Point3D> Shape::getPointsOccupiedAndAdjacentAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	auto output = getPointsOccupiedAt(id, space, location, facing);
	output.maybeInsertAll(getPointsWhichWouldBeAdjacentAt(id, space, location, facing));
	return output;
}
SmallSet<std::pair<Point3D, CollisionVolume>> Shape::getPointsOccupiedAtWithVolumes(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	SmallSet<std::pair<Point3D, CollisionVolume>> output;
	output.reserve(shapeData.m_positions[id].size());
	for(const auto& pair : positionsWithFacing(id, facing))
	{
		Point3D point = location.applyOffset(pair.offset);
		if(space.getAll().contains(point))
			output.emplace(point, pair.volume);
	}
	return output;
}
SmallSet<Point3D> Shape::getPointsWhichWouldBeAdjacentAt(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing)
{
	SmallSet<Point3D> output;
	output.reserve(shapeData.m_positions[id].size());
	for(const Offset3D& offset : shapeData.m_adjacentOffsetsCache[id][(uint)facing])
	{
		Point3D point = location.applyOffset(offset);
		if(space.getAll().contains(point))
			output.insert(point);
	}
	return output;
}
Point3D Shape::getPointWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)> predicate)
{
	for(const auto& pair : shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		Point3D point = location.applyOffset(pair.offset);
		if(space.getAll().contains(point) && predicate(point))
			return point;
	}
	return Point3D::null();
}
Point3D Shape::getPointWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Space& space, const Point3D& location, const Facing4& facing, std::function<bool(const Point3D&)> predicate)
{
	for(const Offset3D& offset : shapeData.m_adjacentOffsetsCache[id][(uint)facing])
	{
		Point3D point = location.applyOffset(offset);
		if(space.getAll().contains(point) && predicate(point))
			return point;
	}
	return Point3D::null();
}
CollisionVolume Shape::getCollisionVolumeAtLocation(const ShapeId& id) { return shapeData.m_positions[id].front().volume; }
CollisionVolume Shape::getTotalCollisionVolume(const ShapeId& id)
{
	if(!getIsMultiTile(id))
		return getCollisionVolumeAtLocation(id);
	else
	{
		CollisionVolume output = CollisionVolume::create(0);
		for(const auto& [offset, volume] : getPositions(id))
		{
			output += volume;
		}
		return output;
	}
}
ShapeId Shape::byName(const std::string& name)
{
	auto found = shapeData.m_name.find(name);
	if(found == shapeData.m_name.end())
		return loadFromName(name);
	return ShapeId::create(found - shapeData.m_name.begin());
}
SmallSet<OffsetAndVolume> Shape::getPositions(const ShapeId& id) { return shapeData.m_positions[id]; }
std::string Shape::getName(const ShapeId& id) { return shapeData.m_name[id]; }
uint32_t Shape::getDisplayScale(const ShapeId& id) { return shapeData.m_displayScale[id]; }
bool Shape::getIsMultiTile(const ShapeId& id) { return shapeData.m_isMultiTile[id]; }
bool Shape::getIsRadiallySymetrical(const ShapeId& id) { return shapeData.m_isRadiallySymetrical[id]; }
// TODO: cache this.
Distance Shape::getZSize(const ShapeId& id)
{
	Distance output = Distance::create(0);
	for(const OffsetAndVolume& offsetAndVolume : getPositions(id))
		if(output < offsetAndVolume.offset.z())
			output = Distance::create(offsetAndVolume.offset.z());
	return output;
}
// TODO: cache this.
SmallSet<OffsetAndVolume> Shape::getPositionsByZLevel(const ShapeId& id, const Distance& zLevel)
{
	SmallSet<OffsetAndVolume> output;
	for(const OffsetAndVolume& offsetAndVolume : getPositions(id))
		if(zLevel == offsetAndVolume.offset.z())
			output.insert(offsetAndVolume);
	return output;
}
Quantity Shape::getNumberOfPointsOnLeadingFaceAtOrBelowLevel(const ShapeId& id, const Distance& zLevel)
{
	SmallSet<std::pair<int, int>> everyYAndZ;
	for(const auto& offsetAndVolume : shapeData.m_positions[id])
	{
		const auto& offset = offsetAndVolume.offset;
		if(zLevel <= offset.z())
			everyYAndZ.maybeInsert({offset.y(), offset.z()});
	}
	return Quantity::create(everyYAndZ.size());
}
bool Shape::hasShape(const std::string& name)
{
	auto found = shapeData.m_name.find(name);
	return found != shapeData.m_name.end();
}
ShapeId Shape::loadFromName(std::string name)
{
	std::vector<int> tokens;
	std::stringstream ss(name);
	std::string item;

	while(getline(ss, item, ' '))
		tokens.push_back(stoi(item));
	assert(tokens.size() % 4 == 0);
	SmallSet<OffsetAndVolume> positions;
	for(size_t i = 0; i < tokens.size(); i+=4)
	{
		int x = tokens[i];
		int y = tokens[i+1];
		int z = tokens[i+2];
		int v = tokens[i+3];
		positions.insert(OffsetAndVolume{Offset3D::create(x, y, z), CollisionVolume::create(v)});
	}
	// Runtime shapes always have display scale = 1
	return Shape::create(name, positions, 1);
}
ShapeId Shape::mutateAdd(const ShapeId& id, const OffsetAndVolume& position)
{
	return mutateAddMultiple(id, {position});
}
ShapeId Shape::mutateAddMultiple(const ShapeId& shape, const SmallSet<OffsetAndVolume>& positions)
{
	// Make a copy.
	SmallSet<OffsetAndVolume> merged = shapeData.m_positions[shape];
	merged.insertAll(positions);
	return createCustom(merged);
}
ShapeId Shape::mutateRemove(const ShapeId& id, const OffsetAndVolume& position)
{
	SmallSet<OffsetAndVolume> positions = {position};
	return mutateRemoveMultiple(id, positions);
}
ShapeId Shape::mutateRemoveMultiple(const ShapeId& id, SmallSet<OffsetAndVolume>& positions)
{
	// Make a copy.
	SmallSet<OffsetAndVolume> purged = shapeData.m_positions[id];
	purged.eraseAll(positions);
	purged.sort();
	return createCustom(purged);
}
ShapeId Shape::mutateMultiplyVolume(const ShapeId& id, const Quantity& quantity)
{
	// Make a copy.
	SmallSet<OffsetAndVolume> copy = shapeData.m_positions[id];
	for(OffsetAndVolume& offsetAndVolume : copy)
		offsetAndVolume.volume *= quantity.get();
	return createCustom(copy);
}
std::string Shape::makeName(SmallSet<OffsetAndVolume>& positions)
{
	std::string output;
	for(const auto& pair : positions)
		output += std::to_string(pair.offset.x()) + " " + std::to_string(pair.offset.y()) + " " + std::to_string(pair.offset.z()) + " " + std::to_string(pair.volume.get()) + " ";
	return output;
}
ShapeId Shape::createCustom(SmallSet<OffsetAndVolume>& positions)
{
	positions.sort();
	std::string name = makeName(positions);
	ShapeId output = Shape::byName(name);
	if(output.exists())
		return output;
	// Runtime shapes always have display scale = 1
	return Shape::create(name, positions, 1);
}