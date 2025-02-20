#include "pointSet.h"
#include "point3D.h"
#include "cuboid.h"
#include "sphere.h"
#include "../config.h"
void Point3DSet::insert(const Point3D& point)
{
	if(m_size == data.size())
		data.conservativeResize(3, std::max(8u, m_size * 2u));
	data.col(m_size) = point.data;
	++m_size;
}
Point3D Point3DSet::operator[](const uint& index) const { assert(index < m_size); return Coordinates(data.col(index)); }
Cuboid Point3DSet::boundry() const
{
	auto truncatedData = data.block(0, 0, 3, m_size);
	Point3D max(truncatedData.rowwise().maxCoeff());
	Point3D min(truncatedData.rowwise().minCoeff());
	return {max, min};
}
Eigen::Array<bool, 1, Eigen::Dynamic> Point3DSet::indicesOfContainedPoints(const Cuboid& cuboid) const
{
	auto truncatedData = data.block(0, 0, 3, m_size);
	return (
		(truncatedData <= cuboid.m_highest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedData >= cuboid.m_lowest.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> Point3DSet::indicesOfContainedPoints(const Sphere& sphere) const
{
	return distancesSquare(sphere.center) <= (int)sphere.radiusSquared().get();
}
Eigen::Array<bool, 1, Eigen::Dynamic> Point3DSet::indicesOfFacedTwordsPoints(const Point3D& location, const Facing4& facing) const
{
	auto truncatedData = data.block(0, 0, 3, m_size);
	switch(facing)
	{
		case Facing4::North:
			return truncatedData.row(1) <= location.y().get();
			break;
		case Facing4::East:
			return truncatedData.row(0) >= location.x().get();
			break;
		case Facing4::South:
			return truncatedData.row(1) >= location.y().get();
			break;
		case Facing4::West:
			return truncatedData.row(0) <= location.x().get();
			break;
		default:
			std::unreachable();
	}
}
Eigen::Array<bool, 1, Eigen::Dynamic> Point3DSet::indicesFacingTwords(const Point3D& location, const Eigen::Array<Facing4, 1, Eigen::Dynamic>& facings) const
{
	assert(facings.size() == m_size);
	auto truncatedData = data.block(0, 0, 3, m_size);
	return (
		(facings == Facing4::North && truncatedData.row(1) >= location.y().get()) ||
		(facings == Facing4::East && truncatedData.row(0) <= location.x().get()) ||
		(facings == Facing4::South && truncatedData.row(1) <= location.y().get()) ||
		(facings == Facing4::West && truncatedData.row(0) >= location.x().get())
	);
}
Eigen::Array<int, 1, Eigen::Dynamic> Point3DSet::distancesSquare(const Point3D& location) const
{
	Eigen::Array<int, 3, Eigen::Dynamic> truncatedData = data.block(0, 0, 3, m_size).cast<int>();
	Eigen::Array<int, 3, Eigen::Dynamic> replicatedLocation = location.data.cast<int>().replicate(1, m_size);
	return (truncatedData - replicatedLocation).square().colwise().sum();
}
Point3D Point3DSet::ConstIterator::operator*() const { return m_set[m_index]; }
