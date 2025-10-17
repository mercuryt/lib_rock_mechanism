#include "setOfPointsHelper.h"
Distance setOfPointsHelper::distanceToClosestSquared(const SmallSet<Point3D>& pointSet, const Point3D& point)
{
	Distance output = Distance::max();
	for(const Point3D& otherPoint : pointSet)
	{
		Distance distance = point.distanceToSquared(otherPoint);
		if(distance == 0)
			return {0};
		if(distance < output)
			output = distance;
	}
	return output;
}