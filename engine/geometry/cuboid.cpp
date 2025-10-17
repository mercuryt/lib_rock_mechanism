#include "cuboid.h"
#include "sphere.h"
#include "offsetCuboid.h"
#include "cuboidSet.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
#include "../space/space.h"
#include "../space/adjacentOffsets.h"

#include <cassert>
Cuboid::Cuboid(const Point3D& highest, const Point3D& lowest) : m_high(highest), m_low(lowest)
{
	if(!m_high.exists())
	{
		assert(!m_low.exists());
		return;
	}
	assert((m_high.data >= m_low.data).all());
}
SmallSet<Point3D> Cuboid::toSet() const
{
	SmallSet<Point3D> output;
	for(const Point3D& point : (*this))
		output.insert(point);
	return output;
}
bool Cuboid::contains(const Point3D& point) const
{
	if(!m_high.exists())
	{
		assert(!m_low.exists());
		return false;
	}
	assert(m_low.exists());
	const bool highResult = (m_high.data >= point.data).all();
	const bool lowResult = (m_low.data <= point.data).all();
	return highResult && lowResult;
}
bool Cuboid::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_high) && contains(cuboid.m_low);
}
bool Cuboid::contains(const Offset3D& offset) const
{
	assert(offset.exists());
	return
	(
		(offset.data >= m_low.data.cast<OffsetWidth>()).all() &&
		(offset.data <= m_high.data.cast<OffsetWidth>()).all()
	);
}
bool Cuboid::contains(const OffsetCuboid& cuboid) const
{
	return contains(cuboid.m_high) && contains(cuboid.m_low);
}
bool Cuboid::canMerge(const Cuboid& other) const
{
	// Can merge requires that the two cuboids share 2 out of 3 axies of symetry.
	assert(isTouching(other));
	auto high = other.m_high.data == m_high.data;
	auto low = other.m_low.data == m_low.data;
	auto sum = high && low;
	uint32_t count = sum.count();
	assert(count != 3);
	return count == 2;
}
Cuboid Cuboid::canMergeSteal(const Cuboid& other) const
{
	// Can merge requires that the two cuboids share 2 out of 3 axies of symetry.
	// can merge steal allows the stolen from cuboid (other) to be larger then the face presented to it by the expanding cuboid.
	Facing6 facing = getFacingTwordsOtherCuboid(other);
	// Get face adjacent to other.
	Cuboid face = getFace(facing);
	// Shift the face one unit into other.
	face.shift(facing, Distance::create(0));
	assert(canMerge(face));
	// If other does not fully contain the face then we cannot steal.
	if(!other.contains(face))
		return {};
	// Find another face on the opposite side of other by copying face and shifting it by other's width.
	Distance depth = other.dimensionForFacing(facing);
	if(depth == 1)
		return face;
	auto face2 = face;
	// Subtract 1 because face is already inside other.
	face2.shift(facing, depth - 1);
	// Combine the two faces and return.
	Cuboid output;
	if(face.m_high >= face2.m_high)
	{
		assert(face2.m_low <= face.m_low);
		output = {face.m_high, face2.m_low};
	}
	else
	{
		assert(face.m_low <= face2.m_low);
		output = {face2.m_high, face.m_low};
	}
	assert(canMerge(output));
	return output;
}
Cuboid Cuboid::sum(const Cuboid& other) const
{
	assert(canMerge(other));
	return { {m_high.data.max(other.m_high.data)}, {m_low.data.min(other.m_low.data)} };
}
OffsetCuboid Cuboid::difference(const Point3D& other) const
{
	return OffsetCuboid::create(m_high.toOffset() - other, m_low.toOffset() - other);
}
Cuboid Cuboid::intersection(const Cuboid& other) const
{
	assert(intersects(other));
	return { {m_high.data.min(other.m_high.data)}, {m_low.data.max(other.m_low.data)} };
}
Cuboid Cuboid::intersection(const OffsetCuboid& other) const
{
	return Cuboid::create(other.intersection(OffsetCuboid::create(*this)));
}
Cuboid Cuboid::intersection(const Point3D& point) const
{
	if(contains(point))
		return {point, point};
	else
		return {};
}
Point3D Cuboid::intersectionPoint(const Point3D& point) const { return contains(point) ? point : Point3D::null(); }
Point3D Cuboid::intersectionPoint(const Cuboid& cuboid) const
{
	assert(intersects(cuboid));
	return intersection(cuboid).m_high;
}
Point3D Cuboid::intersectionPoint(const CuboidSet& cuboids) const
{
	assert(cuboids.intersects(*this));
	for(const Cuboid& other : cuboids)
		if(intersects(other))
			return intersection(other).m_high;
	std::unreachable();
}
OffsetCuboid Cuboid::above() const
{
	return {m_high.above(), Offset3D::create(m_low.x().get(), m_low.y().get(), m_high.z().get() + 1) };
}
void Cuboid::merge(const Cuboid& cuboid)
{
	Cuboid sum = cuboid.sum(*this);
	m_high = sum.m_high;
	m_low = sum.m_low;
}
void Cuboid::setFrom(const Point3D& point)
{
	m_high = m_low = point;
}
void Cuboid::setFrom(const Point3D& a, const Point3D& b)
{
	Cuboid result = fromPointPair(a, b);
	m_high = result.m_high;
	m_low = result.m_low;
}
void Cuboid::setFrom(const Offset3D& high, const Offset3D& low)
{
	assert((low.data >= 0).all());
	(*this) = fromPointPair(Point3D::create(high), Point3D::create(low));
}
void Cuboid::shift(const Facing6& direction, const Distance& distance)
{
	// TODO: make offsetsListDirectlyAdjacent return an Offset3D.
	Offset3D offset = adjacentOffsets::direct[(uint)direction];
	shift(offset, distance);
}
void Cuboid::shift(const Offset3D& offset, const Distance& distance)
{
	assert(distance != 0);
	auto offsetModified = offset;
	offsetModified *= distance;
	m_high += offsetModified;
	m_low += offsetModified;
}
void Cuboid::rotateAroundPoint(const Point3D& point, const Facing4& facing)
{
	Offset3D lowOffset = point - m_low;
	Offset3D highOffset = point - m_high;
	const Offset3D& newLow = point.offsetRotated(lowOffset, facing);
	const Offset3D& newHigh = point.offsetRotated(highOffset, facing);
	setFrom(newLow, newHigh);
}
void Cuboid::setMaxZ(const Distance& distance)
{
	if(m_high.z() > distance)
		m_high.data[2] = distance.get();
}
void Cuboid::maybeExpand(const Cuboid& other)
{
	m_high.data = m_high.data.max(other.m_high.data);
	m_low.data = m_low.data.min(other.m_low.data);
}
void Cuboid::maybeExpand(const Point3D& other)
{
	m_high.data = m_high.data.max(other.data);
	m_low.data = m_low.data.min(other.data);
}
Cuboid Cuboid::inflate(const Distance& distance) const
{
	return {
		m_high + distance,
		m_low.subtractWithMinimum(distance)
	};
}
void Cuboid::clear() { m_low.clear(); m_high.clear(); }
Cuboid Cuboid::getFace(const Facing6& facing) const
{
	switch(facing)
	{
		// test area has x() higher then this.
		case(Facing6::East):
			return Cuboid(m_high, {m_high.x(), m_low.y(), m_low.z()});
		// test area has x() m_lower then this.
		case(Facing6::West):
			return Cuboid({m_low.x(), m_high.y(), m_high.z()}, m_low);
		// test area has y() m_higher then this.
		case(Facing6::South):
			return Cuboid(m_high, {m_low.x(), m_high.y(), m_low.z()});
		// test area has y() m_lower then this.
		case(Facing6::North):
			return Cuboid({m_high.x(), m_low.y(), m_high.z()}, m_low);
		// test area has z() m_higher then this.
		case(Facing6::Above):
			return Cuboid(m_high, {m_low.x(), m_low.y(), m_high.z()});
		// test area has z() m_lower then this.
		case(Facing6::Below):
			return Cuboid({m_high.x(), m_high.y(), m_low.z()}, m_low);
		default:
			assert(false);
			std::unreachable();
	}
}
bool Cuboid::intersects(const Cuboid& other) const
{
	return !(m_high.data < other.m_low.data || m_low.data > other.m_high.data).any();
}
bool Cuboid::overlapsWithSphere(const Sphere& sphere) const
{
	return sphere.intersects(*this);
}
uint Cuboid::volume() const
{
	if(!m_high.exists())
	{
		assert(!m_low.exists());
		return 0;
	}
	assert(m_low.exists());
	return (m_high.data + 1 - m_low.data).prod();
}
// static method
Cuboid Cuboid::fromPoint(const Point3D& point)
{
	return {point, point};
}
Cuboid Cuboid::fromPointPair(const Point3D& a, const Point3D& b)
{
	return { {a.data.max(b.data)}, {a.data.min(b.data)} };
}
Cuboid fromPointSet(const SmallSet<Point3D>& set)
{
	const Point3DSet points = Point3DSet::fromPointSet(set);
	return points.boundry();
}
Cuboid Cuboid::createCube(const Point3D& center, const Distance& width)
{
	return {{center.data + width.get()}, {center.data - width.get()}};
}
Cuboid Cuboid::create(const OffsetCuboid& cuboid)
{
	assert((cuboid.m_low.data >= 0).all());
	return {Point3D(cuboid.m_high.data.cast<DistanceWidth>()), Point3D(cuboid.m_low.data.cast<DistanceWidth>())};
}
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_low == cuboid.m_low && m_high == cuboid.m_high;
}
Point3D Cuboid::getCenter() const
{
	return {(m_low.data + m_high.data) / 2};
}
Distance Cuboid::dimensionForFacing(const Facing6& facing) const
{
	// Add one to output because high/low is an inclusive rather then exclusive range.
	switch(facing)
	{
		case Facing6::Below:
		case Facing6::Above:
			return (m_high.z() - m_low.z()) + 1;
		case Facing6::North:
		case Facing6::South:
			return (m_high.y() - m_low.y()) + 1;
		case Facing6::West:
		case Facing6::East:
			return (m_high.x() - m_low.x()) + 1;
		default:
			std::unreachable();
	}
}
Facing6 Cuboid::getFacingTwordsOtherCuboid(const Cuboid& other) const
{
	assert(!intersects(other));
	if(other.m_high.z() < m_low.z())
		return Facing6::Below;
	if(other.m_low.z() > m_high.z())
		return Facing6::Above;
	if(other.m_high.y() < m_low.y())
		return Facing6::North;
	if(other.m_low.y() > m_high.y())
		return Facing6::South;
	if(other.m_high.x() < m_low.x())
		return Facing6::West;
	assert(other.m_low.x() > m_high.x());
	return Facing6::East;
}
bool Cuboid::isSomeWhatInFrontOf(const Point3D& point, const Facing4& facing) const
{
	return m_high.isInFrontOf(point, facing) || m_low.isInFrontOf(point, facing);
}
bool Cuboid::isTouchingFace(const Cuboid& cuboid) const
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
bool Cuboid::isTouchingFaceFromInside(const Cuboid& cuboid) const
{
	assert(cuboid.contains(*this));
	const Eigen::Array<bool, 3, 1> maxEqualsMax = cuboid.m_high.data == m_high.data;
	const Eigen::Array<bool, 3, 1> minEqualsMin = cuboid.m_low.data == m_low.data;
	return (maxEqualsMax || minEqualsMin).any();
}
SmallSet<Cuboid> Cuboid::getChildrenWhenSplitByCuboid(const Cuboid& cuboid) const
{
	Point3D splitHighest = cuboid.m_high;
	Point3D splitLowest = cuboid.m_low;
	// Clamp split high and low to this so split highest cannot be higher then m_high and split lowest cannot be lower then m_low.
	splitHighest.clampHigh(m_high);
	splitLowest.clampLow(m_low);
	SmallSet<Cuboid> output;
	// Split off group above.
	if(m_high.z() > splitHighest.z())
		output.emplace(m_high, Point3D(m_low.x(), m_low.y(), splitHighest.z() + 1));
	// Split off group below.
	if(m_low.z() < splitLowest.z())
		output.emplace(Point3D(m_high.x(), m_high.y(), splitLowest.z() - 1), m_low);
	// Split off group with higher y()
	if(m_high.y() > splitHighest.y())
		output.emplace(Point3D(m_high.x(), m_high.y(), splitHighest.z()), Point3D(m_low.x(), splitHighest.y() + 1, splitLowest.z()));
	// Split off group with lower y()
	if(m_low.y() < splitLowest.y())
		output.emplace(Point3D(m_high.x(), splitLowest.y() - 1, splitHighest.z()), Point3D(m_low.x(), m_low.y(), splitLowest.z()));
	// Split off group with higher x()
	if(m_high.x() > splitHighest.x())
		output.emplace(Point3D(m_high.x(), splitHighest.y(), splitHighest.z()), Point3D(splitHighest.x() + 1, splitLowest.y(), splitLowest.z()));
	// Split off group with lower x()
	if(m_low.x() < splitLowest.x())
		output.emplace(Point3D(splitLowest.x() - 1, splitHighest.y(), splitHighest.z()), Point3D(m_low.x(), splitLowest.y(), splitLowest.z()));
	return output;
}
std::pair<Cuboid, Cuboid> Cuboid::getChildrenWhenSplitBelowCuboid(const Cuboid& cuboid) const
{
	Point3D splitHighest = cuboid.m_high;
	Point3D splitLowest = cuboid.m_low;
	// Clamp split high and low to this so split highest cannot be higher then m_high and split lowest cannot be lower then m_low.
	splitHighest.clampHigh(m_high);
	splitLowest.clampLow(m_low);
	std::pair<Cuboid, Cuboid> output;
	// Split off group above.
	if(m_high.z() >= splitHighest.z())
		output.first = Cuboid(m_high, Point3D(m_low.x(), m_low.y(), splitHighest.z()));
	// Split off group below.
	if(m_low.z() < splitLowest.z())
		output.second = Cuboid(Point3D(m_high.x(), m_high.y(), splitLowest.z() - 1), m_low);
	return output;
}
bool Cuboid::isTouching(const Cuboid& cuboid) const
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
OffsetCuboid Cuboid::translate(const Point3D& previousPivot, const Point3D& nextPivot, const Facing4& previousFacing, const Facing4& nextFacing) const
{
	return OffsetCuboid::create(m_high.translate(previousPivot, nextPivot, previousFacing, nextFacing), m_low.translate(previousPivot, nextPivot, previousFacing, nextFacing));
}
OffsetCuboid Cuboid::offsetTo(const Point3D& point) const
{
	const Offset3D offsetPoint = point.toOffset();
	return OffsetCuboid::create(m_high.toOffset() - offsetPoint, m_low.toOffset() - offsetPoint);
}
Cuboid::ConstIterator::ConstIterator(const Point3D& lowest, const Point3D& highest)
{
	if(!lowest.exists())
	{
		assert(!highest.exists());
		setToEnd();
	}
	else
	{
		m_current = m_start = lowest;
		m_end = highest;
	}
}
void Cuboid::ConstIterator::setToEnd()
{
	m_start.data.fill(Distance::null().get());
	// TODO: This line isn't needed.
	m_end = m_current = m_start;
}
Cuboid::ConstIterator& Cuboid::ConstIterator::operator=(const ConstIterator& other)
{
	m_start = other.m_start;
	m_end = other.m_end;
	m_current = other.m_current;
	return *this;
}
Cuboid::ConstIterator& Cuboid::ConstIterator::ConstIterator::operator++()
{
	if (m_current.x() < m_end.x()) {
		++m_current.data[0];
	} else if (m_current.y() < m_end.y()) {
		m_current.data[0] = m_start.data[0];
		++m_current.data[1];
	} else if (m_current.z() < m_end.z()) {
		m_current.data[0] = m_start.data[0];
		m_current.data[1] = m_start.data[1];
		++m_current.data[2];
	}
	else
		// End iterator.
		setToEnd();
	return *this;
}
Cuboid::ConstIterator Cuboid::ConstIterator::operator++(int)
{
	Cuboid::ConstIterator tmp = *this;
	++(*this);
	return tmp;
}
const Point3D Cuboid::ConstIterator::operator*() const { return m_current; }
CuboidSurfaceView Cuboid::getSurfaceView() const { return CuboidSurfaceView(*this); }
std::string Cuboid::toString() const { return m_high.toString() + ", " + m_low.toString(); }
CuboidSurfaceView::Iterator::Iterator(const CuboidSurfaceView& v) :
	view(v)
{
	setFace();
}
Cuboid Cuboid::create(const Point3D& high, const Point3D& low)
{
	return {high.max(low), high.min(low)};
}
std::strong_ordering Cuboid::operator<=>(const Cuboid& other) const
{
	return m_high <=> other.m_high;
}
void CuboidSurfaceView::Iterator::setFace()
{
	while(true)
	{
		face = view.cuboid.getFace(facing);
		Offset3D faceLow = face.m_low.toOffset();
		faceLow = faceLow.moveInDirection(facing, Distance::create(1));
		// Skip any face with a low dimension less then zero.
		// Since we don't have Area here we can't skip faces where a high dimension is too high.
		if((faceLow.data >= 0).all())
			break;
		facing = Facing6((uint)facing + 1);
		if(facing == Facing6::Null)
			return;
	}
	current = face.m_low;
}
CuboidSurfaceView::Iterator& CuboidSurfaceView::Iterator::operator++()
{
	if(current.x() < face.m_high.x())
	{
		++current.data[0];
	}
	else if(current.y() < face.m_high.y())
	{
		current.data[0] = face.m_low.data[0];
		++current.data[1];
	}
	else if(current.z() < face.m_high.z())
	{
		current.data[0] = face.m_low.data[0];
		current.data[1] = face.m_low.data[1];
		++current.data[2];
	}
	else if(facing != Facing6::Above)
	{
		facing = (Facing6)((uint)facing + 1);
		setFace();
	}
	else
		// End iterator.
		setToEnd();
	return *this;
}
void CuboidSurfaceView::Iterator::setToEnd() { facing = Facing6::Null; }
CuboidSurfaceView::Iterator CuboidSurfaceView::Iterator::operator++(int)
{
	auto copy = *this;
	++(*this);
	return copy;
}
bool CuboidSurfaceView::Iterator::operator==(const Iterator& other) const
{
	if(facing != other.facing)
		return false;
	if(facing == Facing6::Null)
		return true;
	return current == other.current;
}
std::pair<Point3D, Facing6> CuboidSurfaceView::Iterator::operator*() { return { current, facing }; }