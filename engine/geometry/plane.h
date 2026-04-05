#pragma once
#include "../numericTypes/types.h"

struct Point3D;
struct ParamaterizedLine;
struct Plane
{
	Distance m_location;
	Dimensions m_dimension;
	[[nodiscard]] Point3D intersection(const ParamaterizedLine& line);

};