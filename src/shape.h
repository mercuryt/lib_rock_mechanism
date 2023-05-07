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
	std::vector<std::array<uint32_t, 4>> positions;
	bool isMultiTile;
	Shape(std::string n, std::vector<std::array<uint32_t, 4>>& p, bool imt);
};

static std::list<Shape> s_shapes;

inline Shape::Shape(std::string n, std::vector<std::array<uint32_t, 4>>& p, bool imt) : name(n), isMultiTile(imt)
{
	positions = p;
}

inline const Shape* registerShape(std::string name, std::vector<std::array<uint32_t, 4>> positions)
{
	assert(!positions.empty());
	s_shapes.emplace_back(name, positions, positions.size() != 1);
	//TODO: rotations
	return &s_shapes.back();
}
