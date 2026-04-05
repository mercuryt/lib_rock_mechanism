#include "linePath.h"
#include <ranges>
LinePath LinePath::create(Point3D start, Point3D end)
{
	LinePath output;
	output.m_points.push_back(start);
	output.m_points.push_back(end);
	return output;
}
void LinePath::addWayPoint(Point3D newPoint, Point3D nextPoint)
{
	auto iter = std::ranges::find(m_points, nextPoint);
	assert(iter != m_points.end());
	m_points.insert(iter, newPoint);
}
SmallSet<Point3D> LinePath::toPoints() const
{
	Distance totalDistance{0};
	int end = m_points.size();
	for(int i = 1; i != end; ++i)
		// Add one because distanceTo may round down.
		totalDistance += m_points[i - 1].distanceTo(m_points[i]) + 1;
	SmallSet<Point3D> output;
	output.reserve(totalDistance.get());
	Eigen::Array<float, 3, 1> accumulator = {0.f, 0.f, 0.f};
	for(int i = 1; i != end; ++i)
	{
		ParamaterizedLine line(m_points[i - 1], m_points[i]);
		Point3D point = line.begin;
		while(true)
		{
			output.insert(point);
			if(point == line.end)
				break;
			accumulator += line.sloap;
			for(i = 0; i != 3; ++i)
			{
				if(accumulator[i] >= 1.f)
				{
					accumulator[i] -= 1.f;
					point.data[i] += 1;
				}
				else if(accumulator[i] <= -1.f)
				{
					accumulator[i] += 1.f;
					point.data[i] -= 1;
				}
			}
		}
	}
	return output;
}
ParamaterizedLine LinePath::last() const
{
	return {m_points[m_points.size() - 2], m_points[m_points.size() -1]};
}