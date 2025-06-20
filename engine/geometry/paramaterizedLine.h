#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

#include "../numericTypes/types.h"
#include "point3D.h"
#include "cuboid.h"

struct ParamaterizedLine
{
	Eigen::Array<float, 3, 1> sloap;
	Cuboid boundry;
	Point3D begin;
	Point3D end;

	ParamaterizedLine(const Point3D& b, const Point3D& e) :
		begin(b),
		end(e)
	{

		assert(begin.exists());
		assert(end.exists());
		const Offset3D difference = end.toOffset() - begin.toOffset();
		const DistanceInBlocksFractional distance = end.distanceToFractional(begin);
		sloap = difference.data.cast<float>();
		sloap /= distance.get();
		boundry = {begin.max(end), begin.min(end)};
	}
};