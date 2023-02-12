/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include <vector> 
#include <array> 

struct Shape
{
	std::string name;
	std::vector<std::array<uint32_t, 4>> positions;
	Shape(std::string n, std::vector<std::array<uint32_t, 4>>& p);
};

static std::vector<Shape> s_shapes;

Shape::Shape(std::string n, std::vector<std::array<uint32_t, 4>>& p) : name(n)
{
	positions = p;
}

Shape* registerShape(std::string name, std::vector<std::array<uint32_t, 4>> positions)
{
	s_shapes.emplace_back(name, positions);
	//TODO: rotations
	return &s_shapes.back();
}
