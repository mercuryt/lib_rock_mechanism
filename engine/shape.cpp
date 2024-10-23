#include "shape.h"
#include "blocks/blocks.h"
#include "cuboid.h"
#include "deserializationMemo.h"
#include "types.h"
#include "util.h"
#include <string>
#include <sys/types.h>
//TODO: radial symetry for 2x2 and 3x3, etc.
ShapeId Shape::create(std::string name, std::vector<std::array<int32_t, 4>> positions, uint32_t displayScale)
{
	shapeData.m_name.add(name);
	shapeData.m_positions.add(positions);
	shapeData.m_displayScale.add(displayScale);
	shapeData.m_isMultiTile.add(positions.size() != 1);
	shapeData.m_isRadiallySymetrical.add(positions.size() == 1);
	shapeData.m_occupiedOffsetsCache.add({});
	shapeData.m_adjacentOffsetsCache.add({});
	ShapeId id = ShapeId::create(shapeData.m_name.size() - 1);
	for(Facing i = Facing::create(0); i < 4; ++i)
	{
		shapeData.m_occupiedOffsetsCache[id][i.get()] = makeOccupiedPositionsWithFacing(id, i);
		shapeData.m_adjacentOffsetsCache[id][i.get()] = makeAdjacentPositionsWithFacing(id, i);
	}
	return id;
}
std::vector<std::array<int32_t, 4>> Shape::positionsWithFacing(const ShapeId& id, const Facing& facing) { return shapeData.m_occupiedOffsetsCache[id].at(facing.get()); }
std::vector<std::array<int32_t, 3>> Shape::adjacentPositionsWithFacing(const ShapeId& id, const Facing& facing) { return shapeData.m_adjacentOffsetsCache[id].at(facing.get()); }
std::vector<std::array<int32_t, 4>> Shape::makeOccupiedPositionsWithFacing(const ShapeId& id, const Facing& facing)
{
	//TODO: cache.
	std::vector<std::array<int32_t, 4>> output;
	switch(facing.get())
	{
		case 0: // Facing up.
			return shapeData.m_positions[id];
		case 1: // Facing right, swap x and y.
			for(auto [x, y, z, v] : shapeData.m_positions[id])
				output.push_back({y, x, z, v});
			return output;
		case 2: // Facing down, invert y.
			for(auto [x, y, z, v] : shapeData.m_positions[id])
				output.push_back({x, y * -1, z, v});
			return output;
		case 3: // Facing left, swap x and y and invert x.
			for(auto [x, y, z, v] : shapeData.m_positions[id])
				output.push_back({y * -1, x, z, v});
			return output;
	}
	assert(false);
	return output;
}
std::vector<std::array<int32_t, 3>> Shape::makeAdjacentPositionsWithFacing(const ShapeId& id, const Facing& facing)
{
	std::set<std::array<int32_t, 3>> collect;
	for(auto& position : shapeData.m_occupiedOffsetsCache[id][facing.get()])
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

BlockIndices Shape::getBlocksOccupiedAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing& facing)
{
	BlockIndices output;
	output.reserve(shapeData.m_positions[id].size());
	for(auto [x, y, z, v] : shapeData.m_occupiedOffsetsCache[id][facing.get()])
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		assert(block.exists());
		output.add(block);
	}
	return output;
}
BlockIndices Shape::getBlocksOccupiedAndAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing& facing)
{
	auto output = getBlocksOccupiedAt(id, blocks, location, facing);
	output.concatAssertUnique(getBlocksWhichWouldBeAdjacentAt(id, blocks, location, facing));
	return output;
}
std::vector<std::pair<BlockIndex, CollisionVolume>> Shape::getBlocksOccupiedAtWithVolumes(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing& facing)
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
BlockIndices Shape::getBlocksWhichWouldBeAdjacentAt(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing& facing)
{
	BlockIndices output;
	output.reserve(shapeData.m_positions[id].size());
	for(auto [x, y, z] : shapeData.m_adjacentOffsetsCache[id].at(facing.get()))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists())
			output.add(block);
	}
	return output;
}
BlockIndex Shape::getBlockWhichWouldBeOccupiedAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)> predicate)
{
	for(auto [x, y, z, v] : shapeData.m_occupiedOffsetsCache[id].at(facing.get()))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
BlockIndex Shape::getBlockWhichWouldBeAdjacentAtWithPredicate(const ShapeId& id, const Blocks& blocks, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)> predicate)
{
	for(auto [x, y, z] : shapeData.m_adjacentOffsetsCache[id].at(facing.get()))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
CollisionVolume Shape::getCollisionVolumeAtLocationBlock(const ShapeId& id) { return CollisionVolume::create(shapeData.m_positions[id][0][3]); }
ShapeId Shape::byName(const std::string& name)
{
	auto found = shapeData.m_name.find(name);
	if(found == shapeData.m_name.end())
		return loadFromName(name);
	return ShapeId::create(found - shapeData.m_name.begin());
}
std::vector<std::array<int32_t, 4>> Shape::getPositions(const ShapeId& id) { return shapeData.m_positions[id]; }
std::string Shape::getName(const ShapeId& id) { return shapeData.m_name[id]; }
uint32_t Shape::getDisplayScale(const ShapeId& id) { return shapeData.m_displayScale[id]; }
bool Shape::getIsMultiTile(const ShapeId& id) { return shapeData.m_isMultiTile[id]; }
bool Shape::getIsRadiallySymetrical(const ShapeId& id) { return shapeData.m_isRadiallySymetrical[id]; }
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
	return Shape::create(name, positions, 1);
}
ShapeId Shape::mutateAdd(const ShapeId& id, std::array<int32_t, 4> position)
{
	auto positions = shapeData.m_positions[id];
	assert(std::ranges::find(positions, position) == positions.end());
	positions.push_back(position);
	std::ranges::sort(positions);
	std::string name = makeName(positions);
	ShapeId output = Shape::byName(name);
	if(output.exists())
		return output;
	// Runtime shapes always have display scale = 1
	return Shape::create(name, positions, 1);
}
std::string Shape::makeName(std::vector<std::array<int32_t, 4>>& positions)
{
	std::string output;
	for(auto [x, y, z, v] : positions)
	{
		output.append(std::to_string(x) + " ");
		output.append(std::to_string(y) + " ");
		output.append(std::to_string(z) + " ");
		output.append(std::to_string(v) + " ");
	}
	return output;
}
