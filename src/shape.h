/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include <cassert>
#include <list> 
#include <vector> 
#include <array> 
#include <string>

struct Shape
{
	const std::string name;
	const std::vector<std::array<uint32_t, 4>> positions;
	const bool isMultiTile;
	Shape(const std::string n, const std::vector<std::array<uint32_t, 4>>& p) : name(n), positions(p), isMultiTile(positions.size() != 1) {}
	bool operator==(const Shape& x) const { return &x == this; }
};
