#include "cuboid.h"
#include "sphere.h"
#include "offsetCuboid.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
#include "../space/space.h"
#include "../space/adjacentOffsets.h"

#include <cassert>
Cuboid::Cuboid(const Point3D& highest, const Point3D& lowest) : m_highest(highest), m_lowest(lowest)
{
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return;
	}
	assert((m_highest.data >= m_lowest.data).all());
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
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return false;
	}
	assert(m_lowest.exists());
	return (m_highest.data >= point.data).all() && (m_lowest.data <= point.data).all();
}
bool Cuboid::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_highest) && contains(cuboid.m_lowest);
}
bool Cuboid::contains(const Offset3D& offset) const
{
	assert(offset.exists());
	return
	(
		(offset.data >= m_lowest.data.cast<OffsetWidth>()).all() &&
		(offset.data <= m_highest.data.cast<OffsetWidth>()).all()
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
	auto high = other.m_highest.data == m_highest.data;
	auto low = other.m_lowest.data == m_lowest.data;
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
	face.shift(facing, Distance::create(1));
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
	if(face.m_highest >= face2.m_highest)
	{
		assert(face2.m_lowest <= face.m_lowest);
		output = {face.m_highest, face2.m_lowest};
	}
	else
	{
		assert(face.m_lowest <= face2.m_lowest);
		output = {face2.m_highest, face.m_lowest};
	}
	assert(canMerge(output));
	return output;
}
Cuboid Cuboid::sum(const Cuboid& other) const
{
	assert(canMerge(other));
	return { {m_highest.data.max(other.m_highest.data)}, {m_lowest.data.min(other.m_lowest.data)} };
}
[[nodiscard]] Cuboid Cuboid::intersection(const Cuboid& other) const
{
	return { {m_highest.data.min(other.m_highest.data)}, {m_lowest.data.max(other.m_lowest.data)} };
}
void Cuboid::merge(const Cuboid& cuboid)
{
	Cuboid sum = cuboid.sum(*this);
	m_highest = sum.m_highest;
	m_lowest = sum.m_lowest;
}
void Cuboid::setFrom(const Point3D& point)
{
	m_highest = m_lowest = point;
}
void Cuboid::setFrom(const Point3D& a, const Point3D& b)
{
	Cuboid result = fromPointPair(a, b);
	m_highest = result.m_highest;
	m_lowest = result.m_lowest;
}
void Cuboid::shift(const Facing6& direction, const Distance& distance)
{
	// TODO: make offsetsListDirectlyAdjacent return an Offset3D.
	Offset3D offset = adjacentOffsets::direct[(uint)direction];
	shift(offset, distance);
}
void Cuboid::shift(const Offset3D& offset, const Distance& distance)
{
	auto offsetModified = offset;
	offsetModified *= distance;
	m_highest += offsetModified;
	m_lowest += offsetModified;
}
void Cuboid::rotateAroundPoint(const Point3D& point, const Facing4& facing)
{
	Offset3D lowOffset = point - m_lowest;
	Offset3D highOffset = point - m_highest;
	const Point3D& newLow = point.offsetRotated(lowOffset, facing);
	const Point3D& newHigh = point.offsetRotated(highOffset, facing);
	setFrom(newLow, newHigh);
}
void Cuboid::setMaxZ(const Distance& distance)
{
	if(m_highest.z() > distance)
		m_highest.data[2] = distance.get();
}
void Cuboid::maybeExpand(const Cuboid& other)
{
	m_highest.data = m_highest.data.max(other.m_highest.data);
	m_lowest.data = m_lowest.data.min(other.m_lowest.data);
}
void Cuboid::maybeExpand(const Point3D& other)
{
	m_highest.data = m_highest.data.max(other.data);
	m_lowest.data = m_lowest.data.min(other.data);
}
Cuboid Cuboid::inflateAdd(const Distance& distance) const
{
	return {
		m_highest + distance,
		m_lowest.subtractWithMinimum(distance)
	};
}
void Cuboid::clear() { m_lowest.clear(); m_highest.clear(); }
Cuboid Cuboid::getFace(const Facing6& facing) const
{
	switch(facing)
	{
		// test area has x() higher then this.
		case(Facing6::East):
			return Cuboid(m_highest, {m_highest.x(), m_lowest.y(), m_lowest.z()});
		// test area has x() m_lower then this.
		case(Facing6::West):
			return Cuboid({m_lowest.x(), m_highest.y(), m_highest.z()}, m_lowest);
		// test area has y() m_higher then this.
		case(Facing6::South):
			return Cuboid(m_highest, {m_lowest.x(), m_highest.y(), m_lowest.z()});
		// test area has y() m_lower then this.
		case(Facing6::North):
			return Cuboid({m_highest.x(), m_lowest.y(), m_highest.z()}, m_lowest);
		// test area has z() m_higher then this.
		case(Facing6::Above):
			return Cuboid(m_highest, {m_lowest.x(), m_lowest.y(), m_highest.z()});
		// test area has z() m_lower then this.
		case(Facing6::Below):
			return Cuboid({m_highest.x(), m_highest.y(), m_lowest.z()}, m_lowest);
		default:
			std::unreachable();
		return Cuboid({});
	}
}
bool Cuboid::intersects(const Cuboid& other) const
{
	return !(m_highest.data < other.m_lowest.data || m_lowest.data > other.m_highest.data).any();
}
bool Cuboid::overlapsWithSphere(const Sphere& sphere) const
{
	return sphere.intersects(*this);
}
uint Cuboid::size() const
{
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return 0;
	}
	assert(m_lowest.exists());
	return (m_highest.data + 1 - m_lowest.data).prod();
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
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
Point3D Cuboid::getCenter() const
{
	return {(m_lowest.data + m_highest.data) / 2};
}
Distance Cuboid::dimensionForFacing(const Facing6& facing) const
{
	// Add one to output because high/low is an inclusive rather then exclusive range.
	switch(facing)
	{
		case Facing6::Below:
		case Facing6::Above:
			return (m_highest.z() - m_lowest.z()) + 1;
		case Facing6::North:
		case Facing6::South:
			return (m_highest.y() - m_lowest.y()) + 1;
		case Facing6::West:
		case Facing6::East:
			return (m_highest.x() - m_lowest.x()) + 1;
		default:
			std::unreachable();
	}
}
Facing6 Cuboid::getFacingTwordsOtherCuboid(const Cuboid& other) const
{
	assert(!intersects(other));
	if(other.m_highest.z() < m_lowest.z())
		return Facing6::Below;
	if(other.m_lowest.z() > m_highest.z())
		return Facing6::Above;
	if(other.m_highest.y() < m_lowest.y())
		return Facing6::North;
	if(other.m_lowest.y() > m_highest.y())
		return Facing6::South;
	if(other.m_highest.x() < m_lowest.x())
		return Facing6::West;
	assert(other.m_lowest.x() > m_highest.x());
	return Facing6::East;
}
bool Cuboid::isSomeWhatInFrontOf(const Point3D& point, const Facing4& facing) const
{
	return m_highest.isInFrontOf(point, facing) || m_lowest.isInFrontOf(point, facing);
}
bool Cuboid::isTouchingFace(const Cuboid& cuboid) const
{
	assert(isTouching(cuboid));
	assert(!contains(cuboid));
	const Eigen::Array<bool, 3, 1> maxEqualsMin = cuboid.m_lowest.data == m_highest.data;
	const Eigen::Array<bool, 3, 1> minEqualsMax = cuboid.m_highest.data == m_lowest.data;
	const Eigen::Array<bool, 3, 1> equalFaces = maxEqualsMin && minEqualsMax;
	// If Two faces are equal then it is diagonal adjacent.
	// If Three faces are equal then it is corner adjacent.
	// One face is directly adjacent.
	return equalFaces.count() == 1;
}
bool Cuboid::isTouchingFaceFromInside(const Cuboid& cuboid) const
{
	assert(cuboid.contains(*this));
	const Eigen::Array<bool, 3, 1> maxEqualsMax = cuboid.m_highest.data == m_highest.data;
	const Eigen::Array<bool, 3, 1> minEqualsMin = cuboid.m_lowest.data == m_lowest.data;
	return (maxEqualsMax || minEqualsMin).any();
}
SmallSet<Cuboid> Cuboid::getChildrenWhenSplitByCuboid(const Cuboid& cuboid) const
{
	Point3D splitHighest = cuboid.m_highest;
	Point3D splitLowest = cuboid.m_lowest;
	// Clamp split high and low to this so split highest cannot be higher then m_highest and split lowest cannot be lower then m_lowest.
	splitHighest.clampHigh(m_highest);
	splitLowest.clampLow(m_lowest);
	SmallSet<Cuboid> output;
	// Split off group above.
	if(m_highest.z() > splitHighest.z())
		output.emplace(m_highest, Point3D(m_lowest.x(), m_lowest.y(), splitHighest.z() + 1));
	// Split off group below.
	if(m_lowest.z() < splitLowest.z())
		output.emplace(Point3D(m_highest.x(), m_highest.y(), splitLowest.z() - 1), m_lowest);
	// Split off group with higher y()
	if(m_highest.y() > splitHighest.y())
		output.emplace(Point3D(m_highest.x(), m_highest.y(), splitHighest.z()), Point3D(m_lowest.x(), splitHighest.y() + 1, splitLowest.z()));
	// Split off group with lower y()
	if(m_lowest.y() < splitLowest.y())
		output.emplace(Point3D(m_highest.x(), splitLowest.y() - 1, splitHighest.z()), Point3D(m_lowest.x(), m_lowest.y(), splitLowest.z()));
	// Split off group with higher x()
	if(m_highest.x() > splitHighest.x())
		output.emplace(Point3D(m_highest.x(), splitHighest.y(), splitHighest.z()), Point3D(splitHighest.x() + 1, splitLowest.y(), splitLowest.z()));
	// Split off group with lower x()
	if(m_lowest.x() < splitLowest.x())
		output.emplace(Point3D(splitLowest.x() - 1, splitHighest.y(), splitHighest.z()), Point3D(m_lowest.x(), splitLowest.y(), splitLowest.z()));
	return output;
}
std::pair<Cuboid, Cuboid> Cuboid::getChildrenWhenSplitBelowCuboid(const Cuboid& cuboid) const
{
	Point3D splitHighest = cuboid.m_highest;
	Point3D splitLowest = cuboid.m_lowest;
	// Clamp split high and low to this so split highest cannot be higher then m_highest and split lowest cannot be lower then m_lowest.
	splitHighest.clampHigh(m_highest);
	splitLowest.clampLow(m_lowest);
	std::pair<Cuboid, Cuboid> output;
	// Split off group above.
	if(m_highest.z() >= splitHighest.z())
		output.first = Cuboid(m_highest, Point3D(m_lowest.x(), m_lowest.y(), splitHighest.z()));
	// Split off group below.
	if(m_lowest.z() < splitLowest.z())
		output.second = Cuboid(Point3D(m_highest.x(), m_highest.y(), splitLowest.z() - 1), m_lowest);
	return output;
}
bool Cuboid::isTouching(const Cuboid& cuboid) const
{
	// TODO: SIMD.
	if(
		cuboid.m_lowest.x() > m_highest.x() + 1 ||
		cuboid.m_lowest.y() > m_highest.y() + 1 ||
		cuboid.m_lowest.z() > m_highest.z() + 1 ||
		cuboid.m_highest.x() + 1 < m_lowest.x() ||
		cuboid.m_highest.y() + 1 < m_lowest.y() ||
		cuboid.m_highest.z() + 1 < m_lowest.z()
	)
		return false;
	return true;
}
Cuboid::iterator::iterator(const Point3D& lowest, const Point3D& highest)
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
void Cuboid::iterator::setToEnd()
{
	m_start.data.fill(Distance::null().get());
	// TODO: This line isn't needed.
	m_end = m_current = m_start;
}
Cuboid::iterator& Cuboid::iterator::operator=(const iterator& other)
{
	m_start = other.m_start;
	m_end = other.m_end;
	m_current = other.m_current;
	return *this;
}
Cuboid::iterator& Cuboid::iterator::iterator::operator++()
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
Cuboid::iterator Cuboid::iterator::operator++(int)
{
	Cuboid::iterator tmp = *this;
	++(*this);
	return tmp;
}
Point3D Cuboid::iterator::operator*() { return m_current; }
CuboidSurfaceView Cuboid::getSurfaceView() const { return CuboidSurfaceView(*this); }
std::string Cuboid::toString() const { return m_highest.toString() + ", " + m_lowest.toString(); }
CuboidSurfaceView::Iterator::Iterator(const CuboidSurfaceView& v) :
	view(v)
{
	setFace();
}
std::strong_ordering Cuboid::operator<=>(const Cuboid& other) const
{
	return m_highest <=> other.m_highest;
}
void CuboidSurfaceView::Iterator::setFace()
{
	face = view.cuboid.getFace(facing);
	while(facing != Facing6::Null)
	{
		const Point3D& aboveFace = face.m_highest.moveInDirection(facing, Distance::create(1));
		if(aboveFace.exists())
			break;
		facing = Facing6((uint)facing + 1);
		if(facing != Facing6::Null)
			face = view.cuboid.getFace(facing);
	}
	current = face.m_lowest;
}
CuboidSurfaceView::Iterator& CuboidSurfaceView::Iterator::operator++()
{
	if(current.x() < face.m_highest.x())
	{
		++current.data[0];
	}
	else if(current.y() < face.m_highest.y())
	{
		current.data[0] = face.m_lowest.data[0];
		++current.data[1];
	}
	else if(current.z() < face.m_highest.z())
	{
		current.data[0] = face.m_lowest.data[0];
		current.data[1] = face.m_lowest.data[1];
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