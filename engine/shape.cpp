#include "shape.h"
#include "blocks/blocks.h"
#include "geometry/cuboid.h"
#include "deserializationMemo.h"
#include "types.h"
#include "util.h"
#include <string>
#include <sys/types.h>
//TODO: radial symetry for 2x2 and 3x3, etc.
ShapeId Shape::create(std::wstring name, std::vector<std::array<int32_t, 4>> positions, uint32_t displayScale)
{
	shapeData.m_name.add(name);
	shapeData.m_positions.add(positions);
	shapeData.m_displayScale.add(displayScale);
	shapeData.m_isMultiTile.add(positions.size() != 1);
	shapeData.m_isRadiallySymetrical.add(positions.size() == 1);
	shapeData.m_occupiedOffsetsCache.add({});
	shapeData.m_adjacentOffsetsCache.add({});
	ShapeId id = ShapeId::create(shapeData.m_name.size() - 1);
	for(Facing4 i = Facing4::North; i != Facing4::Null; i = Facing4((uint)i + 1))
	{
		shapeData.m_occupiedOffsetsCache[id][(uint)i] = makeOccupiedPositionsWithFacing(id, i);
		shapeData.m_adjacentOffsetsCache[id][(uint)i] = makeAdjacentPositionsWithFacing(id, i);
	}
	return id;
}
std::vector<std::array<int32_t, 4>> Shape::positionsWithFacing(const ShapeId& id, const Facing4& facing) { return shapeData.m_occupiedOffsetsCache[id][(uint)facing]; }
std::vector<std::array<int32_t, 3>> Shape::adjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing) { return shapeData.m_adjacentOffsetsCache[id][(uint)facing]; }
std::vector<std::array<int32_t, 4>> Shape::makeOccupiedPositionsWithFacing(const ShapeId& id, const Facing4& facing)
{
	//TODO: cache.
	std::vector<std::array<int32_t, 4>> output;
	switch(facing)
	{
		case Facing4::North:
			return shapeData.m_positions[id];
		case Facing4::East:
			for(auto [x, y, z, v] : shapeData.m_positions[id])
				output.push_back({y, x, z, v});
			return output;
		case Facing4::South:
			for(auto [x, y, z, v] : shapeData.m_positions[id])
				output.push_back({x, y * -1, z, v});
			return output;
		case Facing4::West:
			for(auto [x, y, z, v] : shapeData.m_positions[id])
				output.push_back({y * -1, x, z, v});
			return output;
		default:
			assert(false);
	}
	assert(false);
	return output;
}
std::vector<std::array<int32_t, 3>> Shape::makeAdjacentPositionsWithFacing(const ShapeId& id, const Facing4& facing)
{
	std::set<std::array<int32_t, 3>> collect;
	for(auto& position : shapeData.m_occupiedOffsetsCache[id][(uint)facing])
		for(auto& offset : positionOffsets(position))
			collect.insert(offset);
	std::vector<std::array<int32_t, 3>> output(collect.begin(), collect.end());
	return output;
}
std::vector<std::array<int32_t, 3>> Shape::positionOffsets(std::array<int32_t, 4> position)
{
	static const int32_t offsetsList[26][3] = {
		{-1,1,-1}, {-1,0,-1}, {-1,-1,-1},
		{0,1,-1}, {0,0,-1}, {0,-1,-1},
		{1,1,-1}, {1,0,-1}, {1,-1,-1},

		{-1,-1,0}, {-1,0,0}, {0,-1,0},
		{1,1,0}, {0,1,0},
		{1,-1,0}, {1,0,0}, {-1,1,0},

		{-1,1,1}, {-1,0,1}, {-1,-1,1},
		{0,1,1}, {0,0,1}, {0,-1,1},
		{1,1,1}, {1,0,1}, {1,-1,1}
	};
	std::vector<std::array<int32_t, 3>> output;
	for(auto& offsets : offsetsList)
		output.push_back({position[0] + offsets[0], position[1] + offsets[1], position[2] + offsets[2]});
	return output;
}

BlockIndices Shape::getBlocksOccupiedAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing)
{
	BlockIndices output;
	output.reserve(shapeData.m_positions[id].size());
	for(auto [x, y, z, v] : shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		assert(block.exists());
		output.add(block);
	}
	return output;
}
BlockIndices Shape::getBlocksOccupiedAndAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing)
{
	auto output = getBlocksOccupiedAt(id, blocks, location, facing);
	output.concatIgnoreUnique(getBlocksWhichWouldBeAdjacentAt(id, blocks, location, facing));
	return output;
}
std::vector<std::pair<BlockIndex, CollisionVolume>> Shape::getBlocksOccupiedAtWithVolumes(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing)
{
	std::vector<std::pair<BlockIndex, CollisionVolume>> output;
	output.reserve(shapeData.m_positions[id].size());
	for(auto& [x, y, z, v] : positionsWithFacing(id, facing))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists())
			output.emplace_back(block, CollisionVolume::create(v));
	}
	return output;
}
BlockIndices Shape::getBlocksWhichWouldBeAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing)
{
	BlockIndices output;
	output.reserve(shapeData.m_positions[id].size());
	for(auto [x, y, z] : shapeData.m_adjacentOffsetsCache[id][(uint)facing])
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists())
			output.add(block);
	}
	return output;
}
BlockIndex Shape::getBlockWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)> predicate)
{
	for(auto [x, y, z, v] : shapeData.m_occupiedOffsetsCache[id][(uint)facing])
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
BlockIndex Shape::getBlockWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)> predicate)
{
	for(auto [x, y, z] : shapeData.m_adjacentOffsetsCache[id][(uint)facing])
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
CollisionVolume Shape::getCollisionVolumeAtLocationBlock(const ShapeId& id) { return CollisionVolume::create(shapeData.m_positions[id][0][3]); }
ShapeId Shape::byName(const std::wstring& name)
{
	auto found = shapeData.m_name.find(name);
	if(found == shapeData.m_name.end())
		return loadFromName(name);
	return ShapeId::create(found - shapeData.m_name.begin());
}
std::vector<std::array<int32_t, 4>> Shape::getPositions(const ShapeId& id) { return shapeData.m_positions[id]; }
std::wstring Shape::getName(const ShapeId& id) { return shapeData.m_name[id]; }
uint32_t Shape::getDisplayScale(const ShapeId& id) { return shapeData.m_displayScale[id]; }
bool Shape::getIsMultiTile(const ShapeId& id) { return shapeData.m_isMultiTile[id]; }
bool Shape::getIsRadiallySymetrical(const ShapeId& id) { return shapeData.m_isRadiallySymetrical[id]; }
bool Shape::hasShape(const std::wstring& name)
{
	auto found = shapeData.m_name.find(name);
	return found != shapeData.m_name.end();
}
ShapeId Shape::loadFromName(std::wstring wname)
{
	std::string name = util::wideStringToString(wname);
	std::vector<int> tokens;
	std::stringstream ss(name);
	std::string item;

	while(getline(ss, item, ' '))
		tokens.push_back(stoi(item));
	assert(tokens.size() % 4 == 0);
	std::vector<std::array<int, 4>> positions;
	for(size_t i = 0; i < tokens.size(); i+=4)
	{
		int x = tokens[i];
		int y = tokens[i+1];
		int z = tokens[i+2];
		int v = tokens[i+3];
		positions.push_back({x, y, z, v});
	}
	// Runtime shapes always have display scale = 1
	return Shape::create(wname, positions, 1);
}
ShapeId Shape::mutateAdd(const ShapeId& id, std::array<int32_t, 4> position)
{
	auto positions = shapeData.m_positions[id];
	assert(std::ranges::find(positions, position) == positions.end());
	positions.push_back(position);
	std::ranges::sort(positions);
	std::wstring name = makeName(positions);
	ShapeId output = Shape::byName(name);
	if(output.exists())
		return output;
	// Runtime shapes always have display scale = 1
	return Shape::create(name, positions, 1);
}
std::wstring Shape::makeName(std::vector<std::array<int32_t, 4>>& positions)
{
	std::wstring output;
	for(auto [x, y, z, v] : positions)
	{
		output.append(std::to_wstring(x) + L" ");
		output.append(std::to_wstring(y) + L" ");
		output.append(std::to_wstring(z) + L" ");
		output.append(std::to_wstring(v) + L" ");
	}
	return output;
}
