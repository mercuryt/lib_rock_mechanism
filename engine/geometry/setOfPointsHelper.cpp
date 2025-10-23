#include "setOfPointsHelper.h"
DistanceSquared setOfPointsHelper::distanceToClosestSquared(const SmallSet<Point3D>& pointSet, const Point3D& point)
{
	DistanceSquared output = DistanceSquared::max();
	for(const Point3D& otherPoint : pointSet)
	{
		DistanceSquared distance = point.distanceToSquared(otherPoint);
		if(distance == 0)
			return {0};
		if(distance < output)
			output = distance;
	}
	return output;
}