#include "paramaterizedLine.h"
#include "../geometry/cuboidSet.h"

CuboidSet ParamaterizedLine::toSet() const
{
	CuboidSet output;
	Eigen::Array3f endF = end.data.cast<float>();
	for(Eigen::Array3f point = begin.data.cast<float>(); (point < endF).any(); point += slope)
		output.add(Point3D{point.cast<DistanceWidth>()});
	return output;
}