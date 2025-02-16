#include "cuboidSetSIMD.h"
#include "sphere.h"
#include "../config.h"
void CuboidSetSIMD::reserve(uint capacity)
{
	if(capacity < m_capacity)
		return;
	m_high.resize(3, capacity);
	m_low.resize(3, capacity);
	m_capacity = capacity;
}
void CuboidSetSIMD::insert(const Cuboid& cuboid)
{
	if(m_size == m_capacity)
		reserve(m_capacity * Config::goldenRatio);
	m_high.col(m_size) = cuboid.m_highest.data;
	m_low.col(m_size) = cuboid.m_lowest.data;
	++m_size;
	if(m_boundingBox.empty())
		m_boundingBox = cuboid;
	else
		m_boundingBox.maybeExpand(cuboid);
}
void CuboidSetSIMD::clear()
{
	m_high.fill(DistanceInBlocks::null().get());
	m_low.fill(DistanceInBlocks::null().get());
	m_size = 0;
	m_boundingBox.clear();
}
bool CuboidSetSIMD::intersects(const Cuboid& cuboid) const
{
	if(!cuboid.intersects(m_boundingBox))
		return false;
	// m_size is equal to the last used index. Ignore anything after it.
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return !(
		(truncatedHighView < cuboid.m_lowest.data.replicate(1, m_size)).any() ||
		(truncatedLowView > cuboid.m_highest.data.replicate(1, m_size)).any()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfIntersectingCuboids(const Cuboid& cuboid)
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView >= cuboid.m_lowest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView <= cuboid.m_highest.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfContainedCuboids(const Cuboid& cuboid)
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView <= cuboid.m_highest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView >= cuboid.m_lowest.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfIntersectingCuboids(const Sphere& sphere)
{
	int radius = sphere.radius.get();
	const Eigen::Array<int, 1, 3>& center = sphere.center.data.cast<int>();
	auto truncatedHighView = m_high.block(0, 0, 3, m_size).cast<int>();
	auto truncatedLowView = m_low.block(0, 0, 3, m_size).cast<int>();
	return !(
		((truncatedHighView + radius) < center.replicate(1, m_size)).colwise().any() ||
		((truncatedLowView - radius) > center.replicate(1, m_size)).colwise().any()
	);
}
