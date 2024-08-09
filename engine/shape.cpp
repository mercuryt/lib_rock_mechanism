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
	data.m_name.add(name);
	data.m_positions.add(positions);
	data.m_displayScale.add(displayScale);
	ShapeId output = ShapeId::create(data.m_name.size() - 1);
	for(Facing i = Facing::create(0); i < 4; ++i)
	{
		data.m_occupiedOffsetsCache.add({});
		data.m_adjacentOffsetsCache.add({});
		data.m_occupiedOffsetsCache.at(output)[i.get()] = (makeOccupiedPositionsWithFacing(output, i));
		data.m_adjacentOffsetsCache.at(output)[i.get()] = (makeAdjacentPositionsWithFacing(output, i));
	}
	return output;
}
std::vector<std::array<int32_t, 4>> Shape::makeOccupiedPositionsWithFacing(ShapeId id, Facing facing)
{
	//TODO: cache.
	std::vector<std::array<int32_t, 4>> output;
	switch(facing.get())
	{
		case 0: // Facing up.
			return data.m_positions.at(id);
		case 1: // Facing right, swap x and y.
			for(auto [x, y, z, v] : data.m_positions.at(id))
				output.push_back({y, x, z, v});
			return output;
		case 2: // Facing down, invert y.
			for(auto [x, y, z, v] : data.m_positions.at(id))
				output.push_back({x, y * -1, z, v});
			return output;
		case 3: // Facing left, swap x and y and invert x.
			for(auto [x, y, z, v] : data.m_positions.at(id))
				output.push_back({y * -1, x, z, v});
			return output;
	}
	assert(false);
	return output;
}
std::vector<std::array<int32_t, 3>> Shape::makeAdjacentPositionsWithFacing(ShapeId id, Facing facing)
{
	std::set<std::array<int32_t, 3>> collect;
	for(auto& position : data.m_occupiedOffsetsCache.at(id).at(facing.get()))
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

BlockIndices Shape::getBlocksOccupiedAt(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing)
{
	BlockIndices output;
	output.reserve(data.m_positions.at(id).size());
	for(auto [x, y, z, v] : data.m_occupiedOffsetsCache.at(id).at(facing.get()))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		assert(block.exists());
		output.add(block);
	}
	return output;
}
std::vector<std::pair<BlockIndex, CollisionVolume>> Shape::getBlocksOccupiedAtWithVolumes(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing)
{
	std::vector<std::pair<BlockIndex, CollisionVolume>> output;
	output.reserve(data.m_positions.at(id).size());
	for(auto& [x, y, z, v] : positionsWithFacing(id, facing))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists())
			output.emplace_back(block, CollisionVolume::create(v));
	}
	return output;
}
BlockIndices Shape::getBlocksWhichWouldBeAdjacentAt(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing)
{
	BlockIndices output;
	output.reserve(data.m_positions.at(id).size());
	for(auto [x, y, z] : data.m_adjacentOffsetsCache.at(id).at(facing.get()))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists())
			output.add(block);
	}
	return output;
}
BlockIndex Shape::getBlockWhichWouldBeAdjacentAtWithPredicate(ShapeId id, const Blocks& blocks, BlockIndex location, Facing facing, std::function<bool(BlockIndex)> predicate)
{
	for(auto [x, y, z] : data.m_adjacentOffsetsCache.at(id).at(facing.get()))
	{
		BlockIndex block = blocks.offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
CollisionVolume Shape::getCollisionVolumeAtLocationBlock(ShapeId id) { return CollisionVolume::create(data.m_positions.at(id)[0][3]); }
ShapeId Shape::byName(const std::string& name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return ShapeId::create(found - data.m_name.begin());
}
bool Shape::hasShape(const std::string& name)
{
	auto found = data.m_name.find(name);
	return found != data.m_name.end();
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
ShapeId Shape::mutateAdd(ShapeId id, std::array<int32_t, 4> position)
{
	auto positions = data.m_positions.at(id);
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
