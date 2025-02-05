#include "cuboid.h"
#include "sphere.h"
#include "../area.h"
#include "../types.h"
#include "../blocks/blocks.h"

#include <cassert>
Cuboid::Cuboid(const Blocks& blocks, const BlockIndex& h, const BlockIndex& l) : Cuboid(blocks.getCoordinates(h), blocks.getCoordinates(l)) { }
Cuboid::Cuboid(const Point3D& highest, const Point3D& lowest) : m_highest(highest), m_lowest(lowest)
{
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return;
	}
	assert((m_highest.data >= m_lowest.data).all());
}
SmallSet<BlockIndex> Cuboid::toSet(const Blocks& blocks) const
{
	SmallSet<BlockIndex> output;
	for(const BlockIndex& block : getView(blocks))
		output.insert(block);
	return output;
}
bool Cuboid::contains(const Blocks& blocks, const BlockIndex& block) const
{
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return false;
	}
	assert(m_lowest.exists());
	Point3D blockPosition = blocks.getCoordinates(block);
	return contains(blockPosition);
}
bool Cuboid::contains(const Point3D& point) const
{
	return (point.data <= m_highest.data).all() && (point.data >= m_lowest.data).all();
}
bool Cuboid::contains(const Cuboid& cuboid) const
{
	return contains(cuboid.m_highest) && contains(cuboid.m_lowest);
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
	face.shift(facing, DistanceInBlocks::create(1));
	assert(canMerge(face));
	// If other does not fully contain the face then we cannot steal.
	if(!other.contains(face))
		return {};
	// Find another face on the opposite side of other by copying face and shifting it by other's width.
	DistanceInBlocks depth = other.dimensionForFacing(facing);
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
void Cuboid::setFrom(const Blocks& blocks, const BlockIndex& block)
{
	m_highest = m_lowest = blocks.getCoordinates(block);
}
void Cuboid::setFrom(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b)
{
	Cuboid result = fromBlockPair(blocks, a, b);
	m_highest = result.m_highest;
	m_lowest = result.m_lowest;
}
void Cuboid::shift(const Facing6& direction, const DistanceInBlocks& distance)
{
	// TODO: make offsetsListDirectlyAdjacent return an Offset3D.
	Offset3D offset = Blocks::offsetsListDirectlyAdjacent[(uint)direction];
	offset *= distance;
	m_highest += offset;
	m_lowest += offset;
}
void Cuboid::setMaxZ(const DistanceInBlocks& distance)
{
	if(m_highest.z() > distance)
		m_highest.data[2] = distance.get();
}
void Cuboid::maybeExpand(const Cuboid& other)
{
	m_highest.clampHigh(other.m_highest);
	m_lowest.clampLow(other.m_lowest);
}
Cuboid Cuboid::inflateAdd(const DistanceInBlocks& distance) const
{
	return {
		m_highest + distance,
		m_lowest - distance
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
			assert(false);
		return Cuboid({});
	}
}
bool Cuboid::overlapsWith(const Cuboid& other) const
{
	return
		(
		 (m_highest.data >= other.m_lowest.data).all() && (m_highest.data <= other.m_highest.data).all()
		) ||
		(
		 (other.m_highest.data >= m_lowest.data).all() && (other.m_highest.data <= m_highest.data).all()
		) ||
		(
		 (m_lowest.data >= other.m_lowest.data).all() && (m_lowest.data <= other.m_highest.data).all()
		) ||
		(
		 (other.m_lowest.data >= m_lowest.data).all() && (other.m_lowest.data <= m_highest.data).all()
		);
}
bool Cuboid::overlapsWithSphere(const Sphere& sphere) const
{
	return sphere.overlapsWith(*this);
}
size_t Cuboid::size() const
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
Cuboid Cuboid::fromBlock(const Blocks& blocks, const BlockIndex& block)
{
	Point3D a = blocks.getCoordinates(block);
	return {a, a};
}
Cuboid Cuboid::fromBlockPair(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b)
{
	Point3D aCoordinates = blocks.getCoordinates(a);
	Point3D bCoordinates = blocks.getCoordinates(b);
	return { {aCoordinates.data.max(bCoordinates.data)}, {aCoordinates.data.min(bCoordinates.data)} };
}
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
Point3D Cuboid::getCenter() const
{
	return {(m_lowest.data + m_highest.data) / 2};
}
DistanceInBlocks Cuboid::dimensionForFacing(const Facing6& facing) const
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
			assert(false);
	}
}
Facing6 Cuboid::getFacingTwordsOtherCuboid(const Cuboid& other) const
{
	assert(!overlapsWith(other));
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
Cuboid::iterator::iterator(const Blocks& blocks, const BlockIndex& lowest, const BlockIndex& highest) : m_blocks(&blocks)
{
	if(!lowest.exists())
	{
		assert(!highest.exists());
		setToEnd();
	}
	else
	{
		m_current = m_start = blocks.getCoordinates(lowest);
		m_end = blocks.getCoordinates(highest);
	}
}
void Cuboid::iterator::setToEnd()
{
	m_start.data.fill(DistanceInBlocks::null().get());
	// TODO: This line isn't needed.
	m_end = m_current = m_start;
}
Cuboid::iterator& Cuboid::iterator::operator=(const iterator& other)
{
	m_start = other.m_start;
	m_end = other.m_end;
	m_current = other.m_current;
	m_blocks = other.m_blocks;
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
BlockIndex Cuboid::iterator::operator*() { return m_blocks->getIndex(m_current); }
CuboidView Cuboid::getView(const Blocks& blocks) const { assert(!empty()); return CuboidView(blocks, *this); }
CuboidSurfaceView Cuboid::getSurfaceView(const Blocks& blocks) const { return CuboidSurfaceView(blocks, *this); }
std::wstring Cuboid::toString() const { return m_highest.toString() + L", " + m_lowest.toString(); }
CuboidSurfaceView::Iterator::Iterator(const CuboidSurfaceView& v) :
	view(v)
{
	setFace();
}
void CuboidSurfaceView::Iterator::setFace()
{
	face = view.cuboid.getFace(facing);
	while(facing != Facing6::Null)
	{
		const BlockIndex& aboveFace = view.blocks.getAtFacing(view.blocks.getIndex(face.m_highest), facing);
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
std::pair<BlockIndex, Facing6> CuboidSurfaceView::Iterator::operator*() { return { view.blocks.getIndex(current), facing}; }