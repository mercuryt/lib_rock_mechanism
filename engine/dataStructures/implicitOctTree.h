#pragma once
#include "../geometry/point3D.h"
namespace ImplicitOctTree
{
	template<typename T>
	T query(const Point3D& point, const std::vector<T>& data, const Point3D& center, const Distance& halfWidth, int8_t octant = 0, const int32_t& index = 0, const int32_t& depth = 0)
	{
		const auto childOctant = center.getOctant(point);
		switch(childOctant)
		{
			case 0:
				center.data -= halfWidth.get();
			...
			case 7:
				center.data += halfWidth.get();

		}
		int32_t newIndex = std::pow(8, depth) + (octant * std::pow(8, depth - 1) ) + childOctant;
	}
}