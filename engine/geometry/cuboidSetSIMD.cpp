#include "cuboidSetSIMD.h"
#include "cuboidSet.h"
#include "sphere.h"
#include "../config/config.h"
CuboidSetSIMD::CuboidSetSIMD(const CuboidSet& contents) { load(contents.m_cuboids.m_data); }
void CuboidSetSIMD::reserve(int32_t capacity)
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
	m_high.col(m_size) = cuboid.m_high.data;
	m_low.col(m_size) = cuboid.m_low.data;
	++m_size;
	if(m_boundingBox.empty())
		m_boundingBox = cuboid;
	else
		m_boundingBox.maybeExpand(cuboid);
}
void CuboidSetSIMD::update(const int32_t index, const Cuboid& cuboid)
{
	assert(!containsAsMember(cuboid));
	m_high.col(index) = cuboid.m_high.data;
	m_low.col(index) = cuboid.m_low.data;
}
void CuboidSetSIMD::erase(const Cuboid& cuboid)
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	auto indices =
		(truncatedHighView == cuboid.m_high.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView == cuboid.m_low.data.replicate(1, m_size)).colwise().all();
	assert(indices.sum() == 1);
	int32_t index = 0;
	while(!indices[index])
		index++;
	erase(index);
}
void CuboidSetSIMD::erase(int32_t index)
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
void CuboidSetSIMD::load(const std::vector<Cuboid>& contents)
{
	assert(empty());
	reserve(contents.size());
	for(const Cuboid& cuboid : contents)
		insert(cuboid);
}
bool CuboidSetSIMD::intersects(const Cuboid& cuboid) const
{
	if(!cuboid.intersects(m_boundingBox))
		return false;
	// m_size is equal to the last used index. Ignore anything after it.
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return !(
		(truncatedHighView < cuboid.m_low.data.replicate(1, m_size)).any() ||
		(truncatedLowView > cuboid.m_high.data.replicate(1, m_size)).any()
	);
}
bool CuboidSetSIMD::containsAsMember(const Cuboid& cuboid) const
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView == cuboid.m_high.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView == cuboid.m_low.data.replicate(1, m_size)).colwise().all()
	).any();
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfIntersectingCuboids(const Cuboid& cuboid) const
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView >= cuboid.m_low.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView <= cuboid.m_high.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfContainedCuboids(const Cuboid& cuboid) const
{
	auto truncatedHighView = m_high.block(0, 0, 3, m_size);
	auto truncatedLowView = m_low.block(0, 0, 3, m_size);
	return (
		(truncatedHighView <= cuboid.m_high.data.replicate(1, m_size)).colwise().all() &&
		(truncatedLowView >= cuboid.m_low.data.replicate(1, m_size)).colwise().all()
	);
}
Eigen::Array<bool, 1, Eigen::Dynamic> CuboidSetSIMD::indicesOfIntersectingCuboids(const Sphere& sphere) const
{
	assert(m_size != 0);
	int32_t radius = sphere.radius.get();
	const Eigen::Array<int32_t, 3, 1>& center = sphere.center.data.cast<int32_t>();
	auto truncatedHighView = m_high.block(0, 0, 3, m_size).cast<int32_t>();
	auto truncatedLowView = m_low.block(0, 0, 3, m_size).cast<int32_t>();
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
void to_json(Json& data, const CuboidSetSIMD& set) { data = set.toVector(); }
void from_json(const Json& data, CuboidSetSIMD& set) { set.load(data.get<std::vector<Cuboid>>()); }