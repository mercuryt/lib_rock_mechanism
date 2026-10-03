#pragma once
#include "../geometry/point3D.h"
#include "../numericTypes/types.h"

class World;

namespace climate
{
	std::array<Percent, 4> humidityBySeason(const World& world, Point3D point);
	std::array<Temperature, 2> temperatureMinAndMax(const World& world, Point3D point);
}