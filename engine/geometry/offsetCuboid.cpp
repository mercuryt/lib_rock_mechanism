#include "offsetCuboid.h"
#include "../dataStructures/smallSet.h"
#include "../space/adjacentOffsets.h"
OffsetCuboid::OffsetCuboid(const Offset3D& high, const Offset3D& low) :
	m_high(high),
	m_low(low)
{
	assert(high >= low);
}
int OffsetCuboid::volume() const
{
	return
		((m_high.x().get() + 1) - m_low.x().get()) *
		((m_high.y().get() + 1) - m_low.y().get()) *
		((m_high.z().get() + 1) - m_low.z().get());
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
bool OffsetCuboid::intersects(const Offset3D& offset) const
{
	return !(m_high.data < offset.data || m_low.data > offset.data).any();
}
bool OffsetCuboid::intersects(const OffsetCuboid& other) const
{
	return !(m_high.data < other.m_low.data || m_low.data > other.m_high.data).any();
}
OffsetCuboid OffsetCuboid::intersection(const OffsetCuboid& other) const
{
	return { {m_high.data.min(other.m_high.data)}, {m_low.data.max(other.m_low.data)} };
}
bool OffsetCuboid::canMerge(const OffsetCuboid& other) const
{
	if(!isTouching(other))
		return false;
	auto high = other.m_high.data == m_high.data;
	auto low = other.m_low.data == m_low.data;
	auto sum = high && low;
	int count = sum.count();
	assert(count != 3);
	return count == 2;
}
bool OffsetCuboid::isTouching(const OffsetCuboid& other) const
{
	return !((m_high.data + 1) < other.m_low.data || m_low.data > (other.m_high.data + 1)).any();
}
bool OffsetCuboid::isTouching(const Offset3D& offset) const
{
	return !((m_high.data + 1) < offset.data || m_low.data > (offset.data + 1)).any();
}
bool OffsetCuboid::isTouchingFace(const OffsetCuboid& cuboid) const
{
	assert(isTouching(cuboid));
	assert(!contains(cuboid));
	// Check for overlap along each axis
	bool overlapX = (cuboid.m_low.x() <= m_high.x() && cuboid.m_high.x() >= m_low.x());
	bool overlapY = (cuboid.m_low.y() <= m_high.y() && cuboid.m_high.y() >= m_low.y());
	bool overlapZ = (cuboid.m_low.z() <= m_high.z() && cuboid.m_high.z() >= m_low.z());
	// X
	if (overlapY && overlapZ && (cuboid.m_high.x() == m_low.x() || cuboid.m_low.x() == m_high.x()))
		return true;
	// Y
	if (overlapX && overlapZ && (cuboid.m_high.y() == m_low.y() || cuboid.m_low.y() == m_high.y()))
		return true;
	// Z
	if (overlapX && overlapY && (cuboid.m_high.z() == m_low.z() || cuboid.m_low.z() == m_high.z()))
		return true;

	return false;
}
bool OffsetCuboid::isTouchingFaceFromInside(const OffsetCuboid& cuboid) const
{
	assert(cuboid.contains(*this));
	const Eigen::Array<bool, 3, 1> maxEqualsMax = cuboid.m_high.data == m_high.data;
	const Eigen::Array<bool, 3, 1> minEqualsMin = cuboid.m_low.data == m_low.data;
	return (maxEqualsMax || minEqualsMin).any();
}
OffsetCuboid OffsetCuboid::translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const
{
	const auto rotatedHigh = m_high.translate(previousPivot, nextPivot, previousFacing, nextFacing);
	const auto rotatedLow = m_low.translate(previousPivot, nextPivot, previousFacing, nextFacing);
	return create(rotatedHigh, rotatedLow);
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
OffsetCuboid OffsetCuboid::relativeToPoint(const Point3D& point) const { return {point + m_high, point + m_low}; }
OffsetCuboid OffsetCuboid::relativeToOffset(const Offset3D& offset) const { return {offset + m_high, offset + m_low}; }
OffsetCuboid OffsetCuboid::above() const
{
	return {m_high.above(), Offset3D(m_low.x(), m_low.y(), m_high.z() + 1) };
}
OffsetCuboid OffsetCuboid::getFace(const Facing6& facing) const
{
	switch(facing)
	{
		// test area has x() higher then this.
		case(Facing6::East):
			return OffsetCuboid(m_high, {m_high.x(), m_low.y(), m_low.z()});
		// test area has x() m_lower then this.
		case(Facing6::West):
			return OffsetCuboid({m_low.x(), m_high.y(), m_high.z()}, m_low);
		// test area has y() m_higher then this.
		case(Facing6::South):
			return OffsetCuboid(m_high, {m_low.x(), m_high.y(), m_low.z()});
		// test area has y() m_lower then this.
		case(Facing6::North):
			return OffsetCuboid({m_high.x(), m_low.y(), m_high.z()}, m_low);
		// test area has z() m_higher then this.
		case(Facing6::Above):
			return OffsetCuboid(m_high, {m_low.x(), m_low.y(), m_high.z()});
		// test area has z() m_lower then this.
		case(Facing6::Below):
			return OffsetCuboid({m_high.x(), m_high.y(), m_low.z()}, m_low);
		default:
			assert(false);
			std::unreachable();
	}
}
bool OffsetCuboid::hasAnyNegativeCoordinates() const
{
	return (m_low.data >=0).all();
}
OffsetCuboid OffsetCuboid::sum(const OffsetCuboid& other) const
{
	OffsetCuboid output = *this;
	output.maybeExpand(other);
	return output;
}
OffsetCuboid OffsetCuboid::difference(const Offset3D& other) const
{
	return create(m_high - other, m_low - other);
}
Offset3D OffsetCuboid::getCenter() const
{
	return {(m_low.data + m_high.data) / 2};
}
void OffsetCuboid::maybeExpand(const OffsetCuboid& other)
{
	m_high.clampHigh(other.m_high);
	m_low.clampLow(other.m_low);
	assert((m_high.data >= m_low.data).all());
}
void OffsetCuboid::inflate(const Distance& distance)
{
	m_high.data += distance.get();
	m_low.data -= distance.get();
}
void OffsetCuboid::shift(const Facing6& direction, const Distance& distance)
{
	// TODO: make offsetsListDirectlyAdjacent return an Offset3D.
	Offset3D offset = adjacentOffsets::direct[(int)direction];
	shift(offset, distance);
}
void OffsetCuboid::shift(const Offset3D& offset, const Distance& distance)
{
	assert(distance != 0);
	auto offsetModified = offset;
	offsetModified *= distance;
	m_high += offsetModified;
	m_low += offsetModified;
}
void OffsetCuboid::rotateAroundPoint(const Offset3D& offset, const Facing4& facing)
{
	assert(false);
	const Point3D point = Point3D::create(offset);
	const Offset3D lowOffset = m_low.translate(point, point, Facing4::North, facing);
	const Offset3D highOffset = m_high.translate(point, point, Facing4::North, facing);
	*this = create(lowOffset, highOffset);
}
void OffsetCuboid::rotate2D(const Facing4& facing)
{
	m_high.rotate2D(facing);
	m_low.rotate2D(facing);
	*this = create(m_high, m_low);
}
void OffsetCuboid::rotate2D(const Facing4& oldFacing, const Facing4& newFacing)
{
	int rotation = (int)newFacing - (int)oldFacing;
	if(rotation < 0)
		rotation += 4;
	rotate2D((Facing4)rotation);
}
void OffsetCuboid::clear()
{
	m_low.clear();
	m_high.clear();
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
		m_current.setX(m_current.x() + 1);
	else
	{
		m_current.setX(m_cuboid->m_low.x());
		if(m_current.y() < m_cuboid->m_high.y())
			m_current.setY(m_current.y() + 1);
		else
		{
			m_current.setY(m_cuboid->m_low.y());
			m_current.setZ(m_current.z() + 1);
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
OffsetCuboid OffsetCuboid::create(const Cuboid& cuboid) { return {cuboid.m_high.toOffset(), cuboid.m_low.toOffset()}; }
OffsetCuboid OffsetCuboid::create(const Cuboid& cuboid, const Point3D& point) { return create(cuboid.m_high.applyOffset(point), cuboid.m_low.applyOffset(point)); }
OffsetCuboid OffsetCuboid::create(const Offset3D& a, const Offset3D& b) { return {a.max(b), a.min(b)}; }