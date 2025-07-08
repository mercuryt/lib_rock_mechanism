#include "cuboidSetSIMD.h"
#include "sphere.h"
#include "../config.h"
void CuboidSetSIMD::reserve(uint capacity)
{
	if(capacity < m_capacity)
		return;
	m_high.conservativeResize(3, capacity);
	m_low.conservativeResize(3, capacity);
	m_capacity = capacity;
}
void CuboidSetSIMD::insert(const Cuboid& cuboid)
{
	if(m_size == m_capacity)
		reserve(m_capacity * 2);
	m_high.col(m_size) = cuboid.m_highest.data;
	m_low.col(m_size) = cuboid.m_lowest.data;
	++m_size;
	if(m_boundingBox.empty())
		m_boundingBox = cuboid;
	else
		m_boundingBox.maybeExpand(cuboid);
}
void CuboidSetSIMD::update(const uint index, const Cuboid& cuboid)
{
	assert(!containsAsMember(cuboid));
	m_high.col(index) = cuboid.m_highest.data;
	m_low.col(index) = cuboid.m_lowest.data;
}
void CuboidSetSIMD::erase(const Cuboid& cuboid)
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	auto indices =
		(truncatedHighView == cuboid.m_highest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView == cuboid.m_lowest.data.replicate(1, m_size)).colwise().all();
	assert(indices.sum() == 1);
	uint index = 0;
	while(!indices[index])
		index++;
	erase(index);
}
void CuboidSetSIMD::erase(uint index)
{
	if(index != m_size - 1)
	{
		m_high.col(index) = m_high.col(m_size - 1);
		m_low.col(index) = m_low.col(m_size - 1);
	}
	--m_size;
}
void CuboidSetSIMD::clear()
{
	m_high.fill(Distance::null().get());
	m_low.fill(Distance::null().get());
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
bool CuboidSetSIMD::containsAsMember(const Cuboid& cuboid) const
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView == cuboid.m_highest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView == cuboid.m_lowest.data.replicate(1, m_size)).colwise().all()
	).any();
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfIntersectingCuboids(const Cuboid& cuboid) const
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView >= cuboid.m_lowest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView <= cuboid.m_highest.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfContainedCuboids(const Cuboid& cuboid) const
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView <= cuboid.m_highest.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView >= cuboid.m_lowest.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfIntersectingCuboids(const Sphere& sphere) const
{
	assert(m_size != 0);
	int radius = sphere.radius.get();
	const Eigen::Array<int, 3, 1>& center = sphere.center.data.cast<int>();
	auto truncatedHighView = m_high.block(0, 0, 3, m_size).cast<int>();
	auto truncatedLowView = m_low.block(0, 0, 3, m_size).cast<int>();
	auto highLessThen = truncatedHighView < (center - radius).replicate(1, m_size);
	auto lowGreaterThen = truncatedLowView > (center + radius).replicate(1, m_size);
	return ! highLessThen.colwise().any() || lowGreaterThen.colwise().any();
}
std::vector<Cuboid> CuboidSetSIMD::toVector() const
{
	std::vector<Cuboid> output(m_size);
	for(const Cuboid& cuboid : (*this))
		output.push_back(cuboid);
	return output;
}