#include "plane.h"
#include "paramaterizedLine.h"
#include "../numericTypes/types.h"

Point3D Plane::intersection(const ParamaterizedLine& line)
{
	int dimensionAsInt = (int)m_dimension;
	int delta = m_location.get() - line.begin.data[dimensionAsInt];
	int steps = delta / line.sloap[dimensionAsInt];
	// If delta is negitive then sloap should be as well.
	assert(steps >= 0);
	Point3D output = line.begin;
	output.data += (line.sloap * steps).cast<DistanceWidth>();
	return output;
}