#include "shape.h"
#include "block.h"
#include "deserializationMemo.h"
#include "util.h"
#include <string>
#include <sys/types.h>
Shape::Shape(const std::string n, std::vector<std::array<int32_t, 4>> p, uint32_t ds) : name(n), positions(p), displayScale(ds), isMultiTile(positions.size() != 1)
{
	for(uint8_t i = 0; i < 4; ++i)
	{
		occupiedOffsetsCache[i] = makeOccupiedPositionsWithFacing(i);
		adjacentOffsetsCache[i] = makeAdjacentPositionsWithFacing(i);
	}
}
// Runtime shapes never have display scale other then 1.
Shape::Shape(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : name(""), displayScale(1), isMultiTile(data["positions"].size() != 1)
{
	for(const Json& position : data["positions"])
	{
		std::array<int32_t, 4> p{position[0].get<int32_t>(), position[1].get<int32_t>(), position[2].get<int32_t>(), position[3].get<int32_t>()};
		positions.emplace_back(p);
	}
}
Json Shape::toJson() const
{
	Json data{{"positions", Json::array()}};
	for(auto& position: positions)
		data["positions"].push_back(Json(position));
	return data;
}
std::vector<std::array<int32_t, 4>> Shape::makeOccupiedPositionsWithFacing(Facing facing) const
{
	//TODO: cache.
	std::vector<std::array<int32_t, 4>> output;
	switch(facing)
	{
		case 0: // Facing up.
			return positions;
		case 1: // Facing right, swap x and y.
			for(auto [x, y, z, v] : positions)
				output.push_back({y, x, z, v});
			return output;
		case 2: // Facing down, invert y.
			for(auto [x, y, z, v] : positions)
				output.push_back({x, y * -1, z, v});
			return output;
		case 3: // Facing left, swap x and y and invert x.
			for(auto [x, y, z, v] : positions)
				output.push_back({y * -1, x, z, v});
			return output;
	}
	assert(false);
}
std::vector<std::array<int32_t, 3>> Shape::makeAdjacentPositionsWithFacing(Facing facing) const
{
	std::set<std::array<int32_t, 3>> collect;
	for(auto& position : occupiedOffsetsCache.at(facing))
		for(auto& offset : positionOffsets(position))
			collect.insert(offset);
	std::vector<std::array<int32_t, 3>> output(collect.begin(), collect.end());
	return output;
}
std::vector<std::array<int32_t, 3>> Shape::positionOffsets(std::array<int32_t, 4> position) const
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

std::vector<Block*> Shape::getBlocksOccupiedAt(const Block& location, Facing facing) const
{
	std::vector<Block*> output;
	output.reserve(positions.size());
	for(auto& [x, y, z, v] : positionsWithFacing(facing))
	{
		Block* block = location.offset(x, y, z);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
std::vector<std::pair<Block*, Volume>> Shape::getBlocksOccupiedAtWithVolumes(const Block& location, Facing facing) const
{
	std::vector<std::pair<Block*, Volume>> output;
	output.reserve(positions.size());
	for(auto& [x, y, z, v] : positionsWithFacing(facing))
	{
		Block* block = location.offset(x, y, z);
		if(block != nullptr)
			output.emplace_back(block, v);
	}
	return output;
}
SimulationHasShapes::SimulationHasShapes(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& shapeData : data)
	{
		Shape shape = Shape(shapeData, deserializationMemo);
		std::string name = makeName(shape.positions);
		m_shapes.insert({name, shape});
		m_namesByShape[&m_shapes.at(name)] = name;
	}
}
void  SimulationHasShapes::loadFromName(std::string name)
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
	m_shapes.try_emplace(name, name, positions, 1);
}
Json SimulationHasShapes::toJson() const
{
	Json data;
	for(auto& pair : m_shapes)
		data.push_back(pair.second);
	return data;
}
const Shape& SimulationHasShapes::mutateAdd(const Shape& shape, std::array<int32_t, 4> position)
{
	auto positions = shape.positions;
	assert(std::ranges::find(positions, position) == positions.end());
	positions.push_back(position);
	std::ranges::sort(positions);
	std::string name = makeName(positions);
	if(m_shapes.contains(name))
		return m_shapes.at(name);
	// Runtime shapes always have display scale = 1
	auto pair = m_shapes.try_emplace(name, name, positions, 1);
	assert(pair.second);
	const Shape& output = pair.first->second;
	m_namesByShape[&output] = name;
	return output;
}
const Shape& SimulationHasShapes::byName(std::string name) const
{
	assert(m_shapes.contains(name));
	return m_shapes.at(name);
}
const std::string SimulationHasShapes::getName(const Shape& shape) const
{
	assert(m_namesByShape.contains(&shape));
	return m_namesByShape.at(&shape);
}
const std::string SimulationHasShapes::makeName(std::vector<std::array<int32_t, 4>> positions) const 
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
