/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include <vector> 
#include <array> 

struct Shape
{
	std::string name;
	std::vector<std::array<uint32_t, 4>> m_positions;
};

static std::vector<Shape> s_shapes;

Shape* registerShape(std::string name, std::vector<std::array<int32_t, 4>> positions)
{
	Shape& shape = s_shapes.emplace_back(name, positions);
	//TODO: rotations
	return &shape;
}
