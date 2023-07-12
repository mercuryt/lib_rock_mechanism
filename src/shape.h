/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include <cassert>
#include <list> 
#include <vector> 
#include <array> 
#include <string>
#include <algorithm>

struct Shape
{
	const std::string name;
	const std::vector<std::array<const uint32_t, 4>> positions;
	const bool isMultiTile;
	Shape(const std::string n, const std::vector<std::array<const uint32_t, 4>>& p) : name(n), positions(p), isMultiTile(positions.size() != 1) {}
	std::vector<std::array<const uint32_t, 4>> positionsWithFacing(uint8_t facing) const
	{
		std::vector<std::array<const uint32_t, 4>> output;
		switch(facing)
		{
			case 0: // Facing up.
				return positions;
			case 1: // Facing right, swap x and y.
				for(auto& [x, y, z, v] : positions)
					output.push_back({y, x, z, v});
				return output;
			case 2: // Facing down, invert y.
				for(auto& [x, y, z, v] : positions)
					output.push_back({x, y * -1, z, v});
				return output;
			case 3: // Facing left, swap x and y and invert x.
				for(auto& [x, y, z, v] : positions)
					output.push_back({y * -1, x, z, v});
				return output;
		}
		assert(false);
	}
	// Infastructure.
	bool operator==(const Shape& x) const { return &x == this; }
	static std::vector<Shape> data;
	static const Shape& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &Shape::name);
		assert(found != data.end());
		return *found;
	}
};
