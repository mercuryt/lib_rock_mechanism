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
	assert(highest.x >= lowest.x);
	assert(highest.y >= lowest.y);
	assert(highest.z >= lowest.z);
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
		return  false;
	}
	assert(m_lowest.exists());
	Point3D blockPosition = blocks.getCoordinates(block);
	return (
			blockPosition.x <= m_highest.x && blockPosition.x >= m_lowest.x &&
			blockPosition.y <= m_highest.y && blockPosition.y >= m_lowest.y &&
			blockPosition.z <= m_highest.z && blockPosition.z >= m_lowest.z
	       );
}
bool Cuboid::contains(const Point3D& point) const
{
	return (
			point.x <= m_highest.x && point.x >= m_lowest.x &&
			point.y <= m_highest.y && point.y >= m_lowest.y &&
			point.z <= m_highest.z && point.z >= m_lowest.z
	);
}
bool Cuboid::contains(const Cuboid& cuboid) const
{
	// TODO: opitimize this.
	return contains(cuboid.m_highest) && contains(cuboid.m_lowest);
}
bool Cuboid::canMerge(const Cuboid& other) const
{
	// Can merge requires that the two cuboids share 2 out of 3 axies of symetry.
	assert(isTouching(other));
	uint32_t count = 0;
	if(other.m_highest.x == m_highest.x && other.m_lowest.x == m_lowest.x)
		count++;
	if(other.m_highest.y == m_highest.y && other.m_lowest.y == m_lowest.y)
		count++;
	if(other.m_highest.z == m_highest.z && other.m_lowest.z == m_lowest.z)
		count++;
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
	Cuboid output;
	DistanceInBlocks maxX = std::max(m_highest.x, other.m_highest.x);
	DistanceInBlocks maxY = std::max(m_highest.y, other.m_highest.y);
	DistanceInBlocks maxZ = std::max(m_highest.z, other.m_highest.z);
	output.m_highest = {maxX, maxY, maxZ};
	DistanceInBlocks minX = std::min(m_lowest.x, other.m_lowest.x);
	DistanceInBlocks minY = std::min(m_lowest.y, other.m_lowest.y);
	DistanceInBlocks minZ = std::min(m_lowest.z, other.m_lowest.z);
	output.m_lowest = {minX, minY, minZ};
	return output;
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
	auto offset = Blocks::offsetsListDirectlyAdjacent[(uint)direction];
	offset[0] *= distance.get();
	offset[1] *= distance.get();
	offset[2] *= distance.get();
	m_highest.x += offset[0];
	m_lowest.x += offset[0];
	m_highest.y += offset[1];
	m_lowest.y += offset[1];
	m_highest.z += offset[2];
	m_lowest.z += offset[2];
}
void Cuboid::setMaxZ(const DistanceInBlocks& distance)
{
	if(m_highest.z > distance)
		m_highest.z = distance;
}
void Cuboid::maybeExpand(const Cuboid& other)
{
	m_highest.clampHigh(other.m_highest);
	m_lowest.clampLow(other.m_lowest);
}
void Cuboid::clear() { m_lowest.clear(); m_highest.clear(); }
Cuboid Cuboid::getFace(const Facing6& facing) const
{
	switch(facing)
	{
		// test area has x higher then this.
		case(Facing6::East):
			return Cuboid(m_highest, {m_highest.x, m_lowest.y, m_lowest.z});
		// test area has x m_lower then this.
		case(Facing6::West):
			return Cuboid({m_lowest.x, m_highest.y, m_highest.z}, m_lowest);
		// test area has y m_higher then this.
		case(Facing6::South):
			return Cuboid(m_highest, {m_lowest.x, m_highest.y, m_lowest.z});
		// test area has y m_lower then this.
		case(Facing6::North):
			return Cuboid({m_highest.x, m_lowest.y, m_highest.z}, m_lowest);
		// test area has z m_higher then this.
		case(Facing6::Above):
			return Cuboid(m_highest, {m_lowest.x, m_lowest.y, m_highest.z});
		// test area has z m_lower then this.
		case(Facing6::Below):
			return Cuboid({m_highest.x, m_highest.y, m_lowest.z}, m_lowest);
		default:
			assert(false);
		return Cuboid({});
	}
}
bool Cuboid::overlapsWith(const Cuboid& other) const
{
	return
		(
		 m_highest.x >= other.m_lowest.x && m_highest.x <= other.m_highest.x &&
		 m_highest.y >= other.m_lowest.y && m_highest.y <= other.m_highest.y &&
		 m_highest.z >= other.m_lowest.z && m_highest.z <= other.m_highest.z
		) ||
		(
		 other.m_highest.x >= m_lowest.x && other.m_highest.x <= m_highest.x &&
		 other.m_highest.y >= m_lowest.y && other.m_highest.y <= m_highest.y &&
		 other.m_highest.z >= m_lowest.z && other.m_highest.z <= m_highest.z
		) ||
		(
		 m_lowest.x >= other.m_lowest.x && m_lowest.x <= other.m_highest.x &&
		 m_lowest.y >= other.m_lowest.y && m_lowest.y <= other.m_highest.y &&
		 m_lowest.z >= other.m_lowest.z && m_lowest.z <= other.m_highest.z
		) ||
		(
		 other.m_lowest.x >= m_lowest.x && other.m_lowest.x <= m_highest.x &&
		 other.m_lowest.y >= m_lowest.y && other.m_lowest.y <= m_highest.y &&
		 other.m_lowest.z >= m_lowest.z && other.m_lowest.z <= m_highest.z
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
	return ((m_highest.x + 1) - m_lowest.x).get() * ((m_highest.y + 1) - m_lowest.y).get() * ((m_highest.z + 1) - m_lowest.z).get();
}
// static method
Cuboid Cuboid::fromBlock(const Blocks& blocks, const BlockIndex& block)
{
	return Cuboid(blocks, block, block);
}
Cuboid Cuboid::fromBlockPair(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b)
{
	Point3D aCoordinates = blocks.getCoordinates(a);
	Point3D bCoordinates = blocks.getCoordinates(b);
	Cuboid output;
	DistanceInBlocks maxX = std::max(aCoordinates.x, bCoordinates.x);
	DistanceInBlocks maxY = std::max(aCoordinates.y, bCoordinates.y);
	DistanceInBlocks maxZ = std::max(aCoordinates.z, bCoordinates.z);
	output.m_highest = {maxX, maxY, maxZ};
	DistanceInBlocks minX = std::min(aCoordinates.x, bCoordinates.x);
	DistanceInBlocks minY = std::min(aCoordinates.y, bCoordinates.y);
	DistanceInBlocks minZ = std::min(aCoordinates.z, bCoordinates.z);
	output.m_lowest = {minX, minY, minZ};
	return output;
}
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
Point3D Cuboid::getCenter() const
{
	return {
		m_lowest.x + std::round(float(m_highest.x.get() - m_lowest.x.get()) / 2.f),
		m_lowest.y + std::round(float(m_highest.y.get() - m_lowest.y.get()) / 2.f),
		m_lowest.z + std::round(float(m_highest.z.get() - m_lowest.z.get()) / 2.f)
	};
}
DistanceInBlocks Cuboid::dimensionForFacing(const Facing6& facing) const
{
	// Add one to output because high/low is an inclusive rather then exclusive range.
	switch(facing)
	{
		case Facing6::Below:
		case Facing6::Above:
			return (m_highest.z - m_lowest.z) + 1;
		case Facing6::North:
		case Facing6::South:
			return (m_highest.y - m_lowest.y) + 1;
		case Facing6::West:
		case Facing6::East:
			return (m_highest.x - m_lowest.x) + 1;
		default:
			assert(false);
	}
}
Facing6 Cuboid::getFacingTwordsOtherCuboid(const Cuboid& other) const
{
	assert(!overlapsWith(other));
	if(other.m_highest.z < m_lowest.z)
		return Facing6::Below;
	if(other.m_lowest.z > m_highest.z)
		return Facing6::Above;
	if(other.m_highest.y < m_lowest.y)
		return Facing6::North;
	if(other.m_lowest.y > m_highest.y)
		return Facing6::South;
	if(other.m_highest.x < m_lowest.x)
		return Facing6::West;
	assert(other.m_lowest.x > m_highest.x);
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
	if(m_highest.z > splitHighest.z)
		output.emplace(m_highest, Point3D(m_lowest.x, m_lowest.y, splitHighest.z + 1));
	// Split off group below.
	if(m_lowest.z < splitLowest.z)
		output.emplace(Point3D(m_highest.x, m_highest.y, splitLowest.z - 1), m_lowest);
	// Split off group with higher Y
	if(m_highest.y > splitHighest.y)
		output.emplace(Point3D(m_highest.x, m_highest.y, splitHighest.z), Point3D(m_lowest.x, splitHighest.y + 1, splitLowest.z));
	// Split off group with lower Y
	if(m_lowest.y < splitLowest.y)
		output.emplace(Point3D(m_highest.x, splitLowest.y - 1, splitHighest.z), Point3D(m_lowest.x, m_lowest.y, splitLowest.z));
	// Split off group with higher X
	if(m_highest.x > splitHighest.x)
		output.emplace(Point3D(m_highest.x, splitHighest.y, splitHighest.z), Point3D(splitHighest.x + 1, splitLowest.y, splitLowest.z));
	// Split off group with lower X
	if(m_lowest.x < splitLowest.x)
		output.emplace(Point3D(splitLowest.x - 1, splitHighest.y, splitHighest.z), Point3D(m_lowest.x, splitLowest.y, splitLowest.z));
	return output;
}
bool Cuboid::isTouching(const Cuboid& cuboid) const
{
	if(
		cuboid.m_lowest.x > m_highest.x + 1 ||
		cuboid.m_lowest.y > m_highest.y + 1 ||
		cuboid.m_lowest.z > m_highest.z + 1 ||
		cuboid.m_highest.x + 1 < m_lowest.x ||
		cuboid.m_highest.y + 1 < m_lowest.y ||
		cuboid.m_highest.z + 1 < m_lowest.z
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
	// TODO: All these assignments arn't needed, just one would be fine.
	m_start.x = m_start.y = m_start.z = m_current.x = m_current.y = m_current.z = m_end.x = m_end.y = m_end.z = DistanceInBlocks::null();
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
	if (m_current.x < m_end.x) {
		++m_current.x;
	} else if (m_current.y < m_end.y) {
		m_current.x = m_start.x;
		++m_current.y;
	} else if (m_current.z < m_end.z) {
		m_current.x = m_start.x;
		m_current.y = m_start.y;
		++m_current.z;
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
CuboidView Cuboid::getView(const Blocks& blocks) const { return CuboidView(blocks, *this); }
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
	if(current.x < face.m_highest.x)
	{
		++current.x;
	}
	else if(current.y < face.m_highest.y)
	{
		current.x = face.m_lowest.x;
		++current.y;
	}
	else if(current.z < face.m_highest.z)
	{
		current.x = face.m_lowest.x;
		current.y = face.m_lowest.y;
		++current.z;
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