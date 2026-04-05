#pragma once
#include "../numericTypes/types.h"
#include "cuboid.h"

struct ParamaterizedLine;
struct Plane;

template<Dimensions dimension>
class Rectangle
{
	Distance m_plane;
	std::pair<Distance, Distance> m_high;
	std::pair<Distance, Distance> m_low;
public:
	[[nodiscard]] bool intercepts(const ParamaterizedLine& line);
};

using RectangleX = Rectangle<Dimensions::X>;
using RectangleY = Rectangle<Dimensions::Y>;
using RectangleZ = Rectangle<Dimensions::Z>;

struct RectangleWithFacing
{
	Cuboid m_cuboid;
	Facing6 m_facing;
	[[nodiscard]] Point3D clampLine(const ParamaterizedLine& line) const;
	[[nodiscard]] Plane getPlane() const;
};