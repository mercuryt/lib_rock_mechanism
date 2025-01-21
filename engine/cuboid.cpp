#include "cuboid.h"
#include "area.h"
#include "types.h"
#include "sphere.h"
#include "blocks/blocks.h"

#include <cassert>
Cuboid::Cuboid(const Blocks& blocks, const BlockIndex& h, const BlockIndex& l) : m_highest(h), m_lowest(l)
{
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return;
	}
	Point3D highestPosition = blocks.getCoordinates(m_highest);
	Point3D lowestPosition = blocks.getCoordinates(m_lowest);
	assert(highestPosition.x >= lowestPosition.x);
	assert(highestPosition.y >= lowestPosition.y);
	assert(highestPosition.z >= lowestPosition.z);
}
SmallSet<BlockIndex> Cuboid::toSet(Blocks& blocks)
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
	Point3D highestPosition = blocks.getCoordinates(m_highest);
	Point3D lowestPosition = blocks.getCoordinates(m_lowest);
	Point3D blockPosition = blocks.getCoordinates(block);
	return (
			blockPosition.x <= highestPosition.x && blockPosition.x >= lowestPosition.x &&
			blockPosition.y <= highestPosition.y && blockPosition.y >= lowestPosition.y &&
			blockPosition.z <= highestPosition.z && blockPosition.z >= lowestPosition.z
	       );
}
bool Cuboid::contains(const Blocks& blocks, const Cuboid& cuboid) const
{
	// TODO: opitimize this.
	return contains(blocks, cuboid.m_highest) && contains(blocks, cuboid.m_lowest);
}
bool Cuboid::canMerge(const Blocks& blocks, const Cuboid& other) const
{
	// Can merge requires that the two cuboids share 2 out of 3 axies of symetry.
	uint32_t count = 0;
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	Point3D otherHighest = blocks.getCoordinates(other.m_highest);
	Point3D otherLowest = blocks.getCoordinates(other.m_lowest);
	if(otherHighest.x == highest.x && otherLowest.x == lowest.x)
		count++;
	if(otherHighest.y == highest.y && otherLowest.y == lowest.y)
		count++;
	if(otherHighest.z == highest.z && otherLowest.z == lowest.z)
		count++;
	assert(count != 3);
	return count == 2;
}
Cuboid Cuboid::canMergeSteal(const Blocks& blocks, const Cuboid& other) const
{
	// Can merge requires that the two cuboids share 2 out of 3 axies of symetry.
	// can merge steal allows the stolen from cuboid (other) to be larger then the face presented to it by the expanding cuboid.
	Facing facing = getFacingTwordsOtherCuboid(blocks, other);
	// Get face adjacent to other.
	Cuboid face = getFace(blocks, facing);
	// Shift the face one unit into other.
	face.shift(blocks, facing, DistanceInBlocks::create(1));
	assert(canMerge(blocks, face));
	// If other does not fully contain the face then we cannot steal.
	if(!other.contains(blocks, face))
		return {};
	// Find another face on the opposite side of other by copying face and shifting it by other's width.
	DistanceInBlocks depth = other.dimensionForFacing(blocks, facing);
	if(depth == 1)
		return face;
	auto face2 = face;
	// Subtract 1 because face is already inside other.
	face2.shift(blocks, facing, depth - 1);
	// Combine the two faces and return.
	Cuboid output;
	if(face.m_highest >= face2.m_highest)
	{
		assert(face2.m_lowest <= face.m_lowest);
		output = {blocks, face.m_highest, face2.m_lowest};
	}
	else
	{
		assert(face.m_lowest <= face2.m_lowest);
		output = {blocks, face2.m_highest, face.m_lowest};
	}
	assert(canMerge(blocks, output));
	return output;
}
Cuboid Cuboid::sum(const Blocks& blocks, const Cuboid& other) const
{
	assert(canMerge(blocks, other));
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	Point3D otherHighest = blocks.getCoordinates(other.m_highest);
	Point3D otherLowest = blocks.getCoordinates(other.m_lowest);
	Cuboid output;
	DistanceInBlocks maxX = std::max(highest.x, otherHighest.x);
	DistanceInBlocks maxY = std::max(highest.y, otherHighest.y);
	DistanceInBlocks maxZ = std::max(highest.z, otherHighest.z);
	output.m_highest = blocks.getIndex({maxX, maxY, maxZ});
	DistanceInBlocks minX = std::min(lowest.x, otherLowest.x);
	DistanceInBlocks minY = std::min(lowest.y, otherLowest.y);
	DistanceInBlocks minZ = std::min(lowest.z, otherLowest.z);
	output.m_lowest = blocks.getIndex({minX, minY, minZ});
	return output;
}
void Cuboid::merge(const Blocks& blocks, const Cuboid& cuboid)
{
	Cuboid sum = cuboid.sum(blocks, *this);
	m_highest = sum.m_highest;
	m_lowest = sum.m_lowest;
}
void Cuboid::setFrom(const BlockIndex& block)
{
	m_highest = m_lowest = block;
}
void Cuboid::setFrom(const Blocks& blocks, const BlockIndex& a, const BlockIndex& b)
{
	Cuboid result = fromBlockPair(blocks, a, b);
	m_highest = result.m_highest;
	m_lowest = result.m_lowest;
}
void Cuboid::shift(const Blocks& blocks, const Facing& direction, const DistanceInBlocks& distance)
{
	auto offset = Blocks::offsetsListDirectlyAdjacent[direction.get()];
	offset[0] *= distance.get();
	offset[1] *= distance.get();
	offset[2] *= distance.get();
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	highest.x += offset[0];
	lowest.x += offset[0];
	highest.y += offset[1];
	lowest.y += offset[1];
	highest.z += offset[2];
	lowest.z += offset[2];
	m_highest = blocks.getIndex(highest);
	m_lowest = blocks.getIndex(lowest);
}
void Cuboid::setMaxZ(const Blocks& blocks, const DistanceInBlocks& distance)
{
	Point3D highest = blocks.getCoordinates(m_highest);
	if(highest.z > distance)
		highest.z = distance;
	m_highest = blocks.getIndex(highest);
}
void Cuboid::clear() { m_lowest.clear(); m_highest.clear(); }
Cuboid Cuboid::getFace(const Blocks& blocks, const Facing& facing) const
{
	assert(facing < 6);
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	// test area has x higher then this.
	if(facing == 4)
		return Cuboid(blocks, m_highest, blocks.getIndex({highest.x, lowest.y, lowest.z}));
	// test area has x lower then this.
	else if(facing == 2)
		return Cuboid(blocks, blocks.getIndex({lowest.x, highest.y, highest.z}), m_lowest);
	// test area has y higher then this.
	else if(facing == 3)
		return Cuboid(blocks, m_highest, blocks.getIndex({lowest.x, highest.y, lowest.z}));
	// test area has y lower then this.
	else if(facing == 1)
		return Cuboid(blocks, blocks.getIndex({highest.x, lowest.y, highest.z}), m_lowest);
	// test area has z higher then this.
	else if(facing == 5)
		return Cuboid(blocks, m_highest, blocks.getIndex({lowest.x, lowest.y, highest.z}));
	// test area has z lower then this.
	else if(facing == 0)
		return Cuboid(blocks, blocks.getIndex({highest.x, highest.y, lowest.z}), m_lowest);
	return Cuboid(blocks, BlockIndex::null(), BlockIndex::null());
}
bool Cuboid::overlapsWith(const Blocks& blocks, const Cuboid& other) const
{
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	Point3D otherHighest = blocks.getCoordinates(other.m_highest);
	Point3D otherLowest = blocks.getCoordinates(other.m_lowest);
	return
		(
		 highest.x >= otherLowest.x && highest.x <= otherHighest.x &&
		 highest.y >= otherLowest.y && highest.y <= otherHighest.y &&
		 highest.z >= otherLowest.z && highest.z <= otherHighest.z
		) ||
		(
		 otherHighest.x >= lowest.x && otherHighest.x <= highest.x &&
		 otherHighest.y >= lowest.y && otherHighest.y <= highest.y &&
		 otherHighest.z >= lowest.z && otherHighest.z <= highest.z
		) ||
		(
		 lowest.x >= otherLowest.x && lowest.x <= otherHighest.x &&
		 lowest.y >= otherLowest.y && lowest.y <= otherHighest.y &&
		 lowest.z >= otherLowest.z && lowest.z <= otherHighest.z
		) ||
		(
		 otherLowest.x >= lowest.x && otherLowest.x <= highest.x &&
		 otherLowest.y >= lowest.y && otherLowest.y <= highest.y &&
		 otherLowest.z >= lowest.z && otherLowest.z <= highest.z
		);
}
bool Cuboid::overlapsWithSphere(const Blocks& blocks, const Sphere& sphere) const
{
	return sphere.overlapsWith(blocks, *this);
}
size_t Cuboid::size(const Blocks& blocks) const
{
	if(!m_highest.exists())
	{
		assert(!m_lowest.exists());
		return 0;
	}
	assert(m_lowest.exists());
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	return ((highest.x + 1) - lowest.x).get() * ((highest.y + 1) - lowest.y).get() * ((highest.z + 1) - lowest.z).get();
}
// static method
Cuboid Cuboid::fromBlock(Blocks& blocks, const BlockIndex& block)
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
	output.m_highest = blocks.getIndex({maxX, maxY, maxZ});
	DistanceInBlocks minX = std::min(aCoordinates.x, bCoordinates.x);
	DistanceInBlocks minY = std::min(aCoordinates.y, bCoordinates.y);
	DistanceInBlocks minZ = std::min(aCoordinates.z, bCoordinates.z);
	output.m_lowest = blocks.getIndex({minX, minY, minZ});
	return output;
}
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
Point3D Cuboid::getCenter(const Blocks& blocks) const
{
	const Point3D highPoint = blocks.getCoordinates(m_highest);
	const Point3D lowPoint = blocks.getCoordinates(m_lowest);
	return {
		lowPoint.x + (highPoint.x - lowPoint.x) / 2,
		lowPoint.y + (highPoint.y - lowPoint.y) / 2,
		lowPoint.z + (highPoint.z - lowPoint.z) / 2
	};
}
DistanceInBlocks Cuboid::dimensionForFacing(const Blocks& blocks, const Facing& facing) const
{
	const Point3D highPoint = blocks.getCoordinates(m_highest);
	const Point3D lowPoint = blocks.getCoordinates(m_lowest);
	// Add one to output because high/low is an inclusive rather then exclusive range.
	switch(facing.get())
	{
		case 0:
		case 5:
			return (highPoint.z - lowPoint.z) + 1;
		case 1:
		case 3:
			return (highPoint.y - lowPoint.y) + 1;
		case 2:
		case 4:
			return (highPoint.x - lowPoint.x) + 1;
		default:
			assert(false);
	}
}
Facing Cuboid::getFacingTwordsOtherCuboid(const Blocks& blocks, const Cuboid& other) const
{
	assert(!overlapsWith(blocks, other));
	Point3D highest = blocks.getCoordinates(m_highest);
	Point3D lowest = blocks.getCoordinates(m_lowest);
	Point3D otherHighest = blocks.getCoordinates(other.m_highest);
	Point3D otherLowest = blocks.getCoordinates(other.m_lowest);
	// Below
	if(otherHighest.z < lowest.z)
		return Facing::create(0);
	// Above
	if(otherLowest.z > highest.z)
		return Facing::create(5);
	// North
	if(otherHighest.y < lowest.y)
		return Facing::create(1);
	// South
	if(otherLowest.y > highest.y)
		return Facing::create(3);
	// West
	if(otherHighest.x < lowest.x)
		return Facing::create(2);
	// East
	assert(otherLowest.x > highest.x);
	return Facing::create(4);
}
Cuboid::iterator::iterator(Blocks& blocks, const BlockIndex& lowest, const BlockIndex& highest) : m_blocks(blocks)
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
BlockIndex Cuboid::iterator::operator*() { return m_blocks.getIndex(m_current); }
CuboidView Cuboid::getView(Blocks& blocks) const { return CuboidView(blocks, *this); }
CuboidSurfaceView Cuboid::getSurfaceView(Blocks& blocks) const { return CuboidSurfaceView(blocks, *this); }
std::wstring Cuboid::toString(const Blocks& blocks) const { return blocks.getCoordinates(m_highest).toString() + L", " + blocks.getCoordinates(m_lowest).toString(); }
void CuboidSurfaceView::Iterator::setFace()
{
	face = view.cuboid.getFace(view.blocks, facing);
	current = view.blocks.getCoordinates(face.m_lowest);
}
CuboidSurfaceView::Iterator& CuboidSurfaceView::Iterator::operator++()
{
	assert(facing.exists());
	Point3D end = view.blocks.getCoordinates(face.m_highest);
	Point3D start = view.blocks.getCoordinates(face.m_lowest);
	if (current.x < end.x) {
		++current.x;
	} else if (current.y < end.y) {
		current.x = start.x;
		++current.y;
	} else if (current.z < end.z) {
		current.x = start.x;
		current.y = start.y;
		++current.z;
	} else if(facing < 5) {
		++facing;
		setFace();
	} else
		// End iterator.
		setToEnd();
	return *this;
}
void CuboidSurfaceView::Iterator::setToEnd() { facing = Facing::null(); }
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
	if(facing.empty())
		return true;
	return current == other.current;
}
std::pair<BlockIndex, Facing> CuboidSurfaceView::Iterator::operator*() { return { view.blocks.getIndex(current), facing}; }