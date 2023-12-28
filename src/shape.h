/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include "config.h"
#include "types.h"

#include <cassert>
#include <list> 
#include <vector> 
#include <array> 
#include <string>
#include <algorithm>
#include <set>

struct Shape
{
	const std::string name;
	std::vector<std::array<int32_t, 4>> positions;
	const bool isMultiTile;
	std::array<std::vector<std::array<int32_t, 4>>,4> occupiedOffsetsCache;
	std::array<std::vector<std::array<int32_t, 3>>,4> adjacentOffsetsCache;
	std::vector<std::array<int32_t, 4>> positionsWithFacing(Facing facing) const { return occupiedOffsetsCache.at(facing); }
	std::vector<std::array<int32_t, 3>> adjacentPositionsWithFacing(Facing facing) const { return adjacentOffsetsCache.at(facing); }
	Shape(const std::string n, std::vector<std::array<int32_t, 4>> p) : name(n), positions(p), isMultiTile(positions.size() != 1)
	{
		for(uint8_t i = 0; i < 4; ++i)
		{
			occupiedOffsetsCache[i] = makeOccupiedPositionsWithFacing(i);
			adjacentOffsetsCache[i] = makeAdjacentPositionsWithFacing(i);
		}
	}
	std::vector<std::array<int32_t, 4>> makeOccupiedPositionsWithFacing(Facing facing) const
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
	std::vector<std::array<int32_t, 3>> makeAdjacentPositionsWithFacing(Facing facing) const
	{
		std::set<std::array<int32_t, 3>> collect;
		for(auto& position : occupiedOffsetsCache.at(facing))
			for(auto& offset : positionOffsets(position))
				collect.insert(offset);
		std::vector<std::array<int32_t, 3>> output(collect.begin(), collect.end());
		return output;
	}
	std::vector<std::array<int32_t, 3>> positionOffsets(std::array<int32_t, 4> position) const
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
	// Infastructure.
	bool operator==(const Shape& x) const { return &x == this; }
	inline static std::vector<Shape> data;
	static const Shape& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &Shape::name);
		assert(found != data.end());
		return *found;
	}
};
inline void to_json(Json& data, const Shape* const& shape){ data = shape->name; }
inline void to_json(Json& data, const Shape& shape){ data = shape.name; }
inline void from_json(const Json& data, const Shape*& shape){ shape = &Shape::byName(data.get<std::string>()); }
