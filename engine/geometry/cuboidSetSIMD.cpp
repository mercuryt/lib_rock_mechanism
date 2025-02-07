#include "cuboidSetSIMD.h"
#include "../config.h"
#include "cube.h"
void CuboidSetSIMD::reserve(uint capacity)
{
	if(capacity < m_capacity)
		return;
	m_highX.resize(capacity);
	m_highY.resize(capacity);
	m_highZ.resize(capacity);
	m_lowX.resize(capacity);
	m_lowY.resize(capacity);
	m_lowZ.resize(capacity);
	m_capacity = capacity;
}
void CuboidSetSIMD::insert(const Cuboid& cuboid)
{
	if(m_size == m_capacity)
		reserve(m_capacity * Config::goldenRatio);
	m_highX[m_size] = cuboid.m_highest.x().get();
	m_highY[m_size] = cuboid.m_highest.y().get();
	m_highZ[m_size] = cuboid.m_highest.z().get();
	m_lowX[m_size] = cuboid.m_lowest.x().get();
	m_lowY[m_size] = cuboid.m_lowest.y().get();
	m_lowZ[m_size] = cuboid.m_lowest.z().get();
	++m_size;
	if(m_boundingBox.empty())
		m_boundingBox = cuboid;
	else
		m_boundingBox.maybeExpand(cuboid);
}
void CuboidSetSIMD::clear()
{
	m_highX.fill(DistanceInBlocks::null().get());
	m_highY.fill(DistanceInBlocks::null().get());
	m_highZ.fill(DistanceInBlocks::null().get());
	m_lowX.fill(DistanceInBlocks::null().get());
	m_lowY.fill(DistanceInBlocks::null().get());
	m_lowZ.fill(DistanceInBlocks::null().get());
	m_size = 0;
	m_boundingBox.clear();
}
bool CuboidSetSIMD::intersects(const Cuboid& cuboid) const
{
	if(!cuboid.intersects(m_boundingBox))
		return false;
	auto highX = m_highX.head(m_size);
	auto highY = m_highY.head(m_size);
	auto highZ = m_highZ.head(m_size);
	auto lowX = m_lowX.head(m_size);
	auto lowY = m_lowY.head(m_size);
	auto lowZ = m_lowZ.head(m_size);
	return
		!(
			highX <= cuboid.m_lowest.x().get() || highY <= cuboid.m_lowest.y().get() || highZ <= cuboid.m_lowest.z().get() ||
			lowX >= cuboid.m_highest.x().get() || lowY >= cuboid.m_highest.y().get() || lowZ >= cuboid.m_highest.z().get()
		).all();
}
bool CuboidSetSIMD::intersects(const Cube& cube) const
{
	return intersects({cube.getHighPoint(), cube.getLowPoint()});
}