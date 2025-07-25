#pragma once
#include "../numericTypes/types.h"

struct ParamaterizedLine;

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