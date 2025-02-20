#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

#include "types.h"

struct ParamaterizedLine
{
	Eigen::Array<float, 1, 6> coefficients;
	ParamaterizedLine(const Point3D& a, const Point3D& b)
	{
		// Disperse to match a single set of dimensions which has been repeated horizontally.
		auto differences = b.data - a.data;
		// for x.
		coefficients[0] = (float)differences.y().get() / (float)differences.x().get();
		coefficients[3] = (float)differences.z().get() / (float)differences.x().get();
		// for y.
		coefficients[1] = (float)differences.x().get() / (float)differences.y().get();
		coefficients[4] = (float)differences.z().get() / (float)differences.y().get();
		// for z.
		coefficients[2] = (float)differences.x().get() / (float)differences.z().get();
		coefficients[5] = (float)differences.y().get() / (float)differences.z().get();
	}
};
struct CubuidSetForIntersectingLine
{
	using Data = Eigen::Array<float, 3, Eigen::Dynamic>;
	Data highPoints;
	Data lowPoints;
	Data axisMergedPoints;
	template<class CuboidSet>
	CuboidSetForIntersectingLine(const CuboidSet& cuboidSet)
	{
		highPoints.conservativeResize(3, cuboidSet.size());
		lowPoints.conservativeResize(3, cuboidSet.size());
		axisMergedPoints.resize(3, cuboidSet.size() * 2);
		uint i = 0;
		for(const Cuboid& cuboid : cuboidSet)
		{
			highPoints.column(i) = cuboid.m_highest.data.cast<float>();
			lowPoints.column(i) = cuboid.m_lowest.data.cast<float>();
			axisMergedPoints.column(i * 2) = highPoints.column(i);
			axisMergedPoints.column((i * 2) + 1) = lowPoints.column(i);
			++i;
		}
	}
	bool intersects(const ParamaterizedLine& line)
	{
		// TODO: maxtrixify.
		auto otherCoordinatesAtIntersections = axisMergedPoints.replicate(2, 1) * line.coefficients;
		auto maskNorth = otherCoordinatesAtIntersections.row(0) >= lowPoints.y.get();
		auto maskSouth = otherCoordinatesAtIntersections.row(0) <= highPoints.y.get();
		auto maskYForX = maskNorth && maskSouth;
		auto maskBelow = otherCoordinatesAtIntersections.row(3) >= lowPoints.z.get();
		auto maskAbove = otherCoordinatesAtIntersections.row(3) <= highPoints.z.get();
		auto maskZForX = maskBelow && maskAbove;
		if((maskZForX && maskYForX).any())
			return true;
		auto maskWest = otherCoordinatesAtIntersections.row(1) >= lowPoints.x.get();
		auto maskEast = otherCoordinatesAtIntersections.row(1) <= highPoints.x.get();
		auto maskXForY = maskNorth && maskSouth;
		auto maskBelow = otherCoordinatesAtIntersections.row(4) >= lowPoints.z.get();
		auto maskAbove = otherCoordinatesAtIntersections.row(4) <= highPoints.z.get();
		auto maskZForY = maskBelow && maskAbove;
		if((maskXForY && maskZForY.any()))
			return true;
		auto maskWest = otherCoordinatesAtIntersections.row(2) >= lowPoints.x.get();
		auto maskEast = otherCoordinatesAtIntersections.row(2) <= highPoints.x.get();
		auto maskXForZ = maskNorth && maskSouth;
		auto maskNorth = otherCoordinatesAtIntersections.row(5) >= lowPoints.y.get();
		auto maskSouth = otherCoordinatesAtIntersections.row(5) <= highPoints.y.get();
		auto maskYForZ = maskBelow && maskAbove;
		if((maskXForZ && maskYForZ).any())
			return true;
		return false;
	}
};