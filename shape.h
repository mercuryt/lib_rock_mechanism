/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include <list> 
#include <vector> 
#include <array> 
#include <string>

struct Shape
{
	const std::string name;
	std::vector<std::array<uint32_t, 4>> positions;
	Shape(std::string n, std::vector<std::array<uint32_t, 4>>& p);
};

static std::list<Shape> s_shapes;

Shape::Shape(std::string n, std::vector<std::array<uint32_t, 4>>& p) : name(n)
{
	positions = p;
}

const Shape* registerShape(std::string name, std::vector<std::array<uint32_t, 4>> positions)
{
	s_shapes.emplace_back(name, positions);
	//TODO: rotations
	return &s_shapes.back();
}
