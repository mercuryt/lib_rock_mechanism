#include "offsetCuboid.h"
#include "../dataStructures/smallSet.h"
OffsetCuboid::OffsetCuboid(const Offset3D& high, const Offset3D& low) :
	m_high(high),
	m_low(low)
{
	assert(high >= low);
}
uint OffsetCuboid::size() const
{
	return
		((m_high.x() + 1) - m_low.x()) *
		((m_high.y() + 1) - m_low.y()) *
		((m_high.z() + 1) - m_low.z());
}
bool OffsetCuboid::contains(const Offset3D& offset) const
{
	return (m_high.data >= offset.data).all() && (m_low.data <= offset.data).all();
}
bool OffsetCuboid::contains(const OffsetCuboid& other) const
{
	return
		(m_high.data >= other.m_high.data).all() && (m_low.data <= other.m_high.data).all() &&
		(m_high.data >= other.m_low.data).all() && (m_low.data <= other.m_low.data).all();
}
bool OffsetCuboid::intersects(const OffsetCuboid& other) const
{
	return
		((m_high.data >= other.m_high.data).all() && (m_low.data <= other.m_high.data).all()) ||
		((m_high.data >= other.m_low.data).all() && (m_low.data <= other.m_low.data).all());
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
SmallSet<OffsetCuboid> OffsetCuboid::getChildrenWhenSplitByCuboid(const OffsetCuboid& cuboid) const
{
	Offset3D splitHigh = cuboid.m_high;
	Offset3D splitLow = cuboid.m_low;
	// Clamp split high and low to this so split high cannot be higher then m_high and split low cannot be lower then m_low.
	splitHigh.clampHigh(m_high);
	splitLow.clampLow(m_low);
	SmallSet<OffsetCuboid> output;
	// Split off group above.
	if(m_high.z() > splitHigh.z())
		output.emplace(m_high, Offset3D(m_low.x(), m_low.y(), splitHigh.z() + 1));
	// Split off group below.
	if(m_low.z() < splitLow.z())
		output.emplace(Offset3D(m_high.x(), m_high.y(), splitLow.z() - 1), m_low);
	// Split off group with higher y()
	if(m_high.y() > splitHigh.y())
		output.emplace(Offset3D(m_high.x(), m_high.y(), splitHigh.z()), Offset3D(m_low.x(), splitHigh.y() + 1, splitLow.z()));
	// Split off group with lower y()
	if(m_low.y() < splitLow.y())
		output.emplace(Offset3D(m_high.x(), splitLow.y() - 1, splitHigh.z()), Offset3D(m_low.x(), m_low.y(), splitLow.z()));
	// Split off group with higher x()
	if(m_high.x() > splitHigh.x())
		output.emplace(Offset3D(m_high.x(), splitHigh.y(), splitHigh.z()), Offset3D(splitHigh.x() + 1, splitLow.y(), splitLow.z()));
	// Split off group with lower x()
	if(m_low.x() < splitLow.x())
		output.emplace(Offset3D(splitLow.x() - 1, splitHigh.y(), splitHigh.z()), Offset3D(m_low.x(), splitLow.y(), splitLow.z()));
	return output;
}
void OffsetCuboid::extend(const OffsetCuboid& other)
{
	m_high.clampHigh(other.m_high);
	m_low.clampLow(other.m_low);
	assert((m_high.data >= m_low.data).all());
}
OffsetCuboid::ConstIterator::ConstIterator(const OffsetCuboid& cuboid, const Offset3D& position) :
	m_cuboid(const_cast<OffsetCuboid*>(&cuboid)),
	m_current(position)
{ }
bool OffsetCuboid::ConstIterator::operator==(const ConstIterator& other) const
{
	assert(m_cuboid == other.m_cuboid);
	return m_current == other.m_current;
}
Offset3D OffsetCuboid::ConstIterator::operator*() const { return m_current; }
OffsetCuboid::ConstIterator OffsetCuboid::ConstIterator::operator++()
{
	// m_current.z == m_cuboid.m_high.z() + 1 is the end state.
	assert(m_current.z() <= m_cuboid->m_high.z());
	if(m_current.x() < m_cuboid->m_high.x())
		++m_current.x();
	else
	{
		m_current.x() = m_cuboid->m_low.x();
		if(m_current.y() < m_cuboid->m_high.y())
			++m_current.y();
		else
		{
			m_current.y() = m_cuboid->m_low.y();
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