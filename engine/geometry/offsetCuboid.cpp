#include "offsetCuboid.h"
OffsetCuboid::OffsetCuboid(const Offset3D& high, const Offset3D& low) :
	m_high(high),
	m_low(low)
{ }
uint OffsetCuboid::size() const
{
	return
		((m_high.x() + 1) - m_low.x()) +
		((m_high.y() + 1) - m_low.y()) +
		((m_high.z() + 1) - m_low.z());
}
bool OffsetCuboid::contains(const Offset3D& offset) const
{
	return m_high >= offset && m_low <= offset;
}
bool OffsetCuboid::contains(const OffsetCuboid& other) const
{
	return
		m_high >= other.m_high && m_low <= other.m_high &&
		m_high >= other.m_low && m_low <= other.m_low;
}
bool OffsetCuboid::intersects(const OffsetCuboid& other) const
{
	return
		(m_high >= other.m_high && m_low <= other.m_high) ||
		(m_high >= other.m_low && m_low <= other.m_low);
}
bool OffsetCuboid::canMerge(const OffsetCuboid& other) const
{
	if(!isTouching(other))
		return false;
	auto high = other.m_high.data == m_high.data;
	auto low = other.m_low.data == m_low.data;
	auto sum = high && low;
	uint32_t count = sum.count();
	assert(count != 3);
	return count == 2;
}
bool OffsetCuboid::isTouching(const OffsetCuboid& cuboid) const
{
	// TODO: SIMD.
	if(
		cuboid.m_low.x() > m_high.x() + 1 ||
		cuboid.m_low.y() > m_high.y() + 1 ||
		cuboid.m_low.z() > m_high.z() + 1 ||
		cuboid.m_high.x() + 1 < m_low.x() ||
		cuboid.m_high.y() + 1 < m_low.y() ||
		cuboid.m_high.z() + 1 < m_low.z()
	)
		return false;
	return true;
}
void OffsetCuboid::extend(const OffsetCuboid& other)
{
	m_high.clampHigh(other.m_high);
	m_low.clampLow(other.m_low);
}
OffsetCuboid::ConstIterator::ConstIterator(const OffsetCuboid& cuboid, const Offset3D& position) :
	m_cuboid(const_cast<OffsetCuboid*>(&cuboid)),
	m_current(position)
{ }
Offset3D OffsetCuboid::ConstIterator::operator*() const
{
	return m_current;
}
OffsetCuboid::ConstIterator OffsetCuboid::ConstIterator::operator++()
{
	if(m_current.x() < m_cuboid->m_high.x())
		++m_current.x();
	else
	{
		m_current.x() = 0;
		if(m_current.y() < m_cuboid->m_high.y())
			++m_current.y();
		else
		{
			// m_current.z == m_cuboid.m_high.z() + 1 is the end state.
			assert(m_current.z() < m_cuboid->m_high.z() + 1);
			++m_current.z();
		}

	}
	return *this;
}
OffsetCuboid::ConstIterator OffsetCuboid::ConstIterator::operator++(int)
{
	auto copy = *this;
	++(*this);
	return copy;
}
Json OffsetCuboid::toJson() const { return {m_high, m_low}; }
void OffsetCuboid::load(const Json& data) { data[0].get_to(m_high); data[1].get_to(m_low); }