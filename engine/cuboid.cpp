#include "cuboid.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"

#include <cassert>
Cuboid::Cuboid(Blocks& b, BlockIndex h, BlockIndex l) : m_blocks(&b), m_highest(h), m_lowest(l)
{
	if(m_highest == BLOCK_INDEX_MAX)
	{
		assert(m_lowest == BLOCK_INDEX_MAX);
		return;
	}
	Point3D highestPosition = m_blocks->getCoordinates(m_highest);
	Point3D lowestPosition = m_blocks->getCoordinates(m_lowest);
	assert(highestPosition.x >= lowestPosition.x);
	assert(highestPosition.y >= lowestPosition.y);
	assert(highestPosition.z >= lowestPosition.z);
}
std::unordered_set<BlockIndex> Cuboid::toSet()
{
	std::unordered_set<BlockIndex> output;
	for(BlockIndex block : *this)
		output.insert(block);
	return output;
}
bool Cuboid::contains(BlockIndex block) const
{
	if(m_highest == BLOCK_INDEX_MAX)
	{
		assert(m_lowest == BLOCK_INDEX_MAX);
		return  false;
	}
	assert(m_lowest != BLOCK_INDEX_MAX);
	Point3D highestPosition = m_blocks->getCoordinates(m_highest);
	Point3D lowestPosition = m_blocks->getCoordinates(m_lowest);
	Point3D blockPosition = m_blocks->getCoordinates(block);
	return (
			blockPosition.x <= highestPosition.x && blockPosition.x >= lowestPosition.x &&
			blockPosition.y <= highestPosition.y && blockPosition.y >= lowestPosition.y &&
			blockPosition.z <= highestPosition.z && blockPosition.z >= lowestPosition.z
	       );
}
bool Cuboid::canMerge(const Cuboid& other) const
{
	uint32_t count = 0;
	Point3D highest = m_blocks->getCoordinates(m_highest);
	Point3D lowest = m_blocks->getCoordinates(m_lowest);
	Point3D otherHighest = m_blocks->getCoordinates(other.m_highest);
	Point3D otherLowest = m_blocks->getCoordinates(other.m_lowest);
	if(otherHighest.x == highest.x && otherLowest.x == lowest.x)
		count++;
	if(otherHighest.y == highest.y && otherLowest.y == lowest.y)
		count++;
	if(otherHighest.z == highest.z && otherLowest.z == lowest.z)
		count++;
	assert(count != 3);
	return count == 2;
}
Cuboid Cuboid::sum(const Cuboid& other) const
{
	assert(canMerge(other));
	Point3D highest = m_blocks->getCoordinates(m_highest);
	Point3D lowest = m_blocks->getCoordinates(m_lowest);
	Point3D otherHighest = m_blocks->getCoordinates(other.m_highest);
	Point3D otherLowest = m_blocks->getCoordinates(other.m_lowest);
	Cuboid output;
	uint32_t maxX = std::max(highest.x, otherHighest.x);
	uint32_t maxY = std::max(highest.y, otherHighest.y);
	uint32_t maxZ = std::max(highest.z, otherHighest.z);
	output.m_highest = m_blocks->getIndex({maxX, maxY, maxZ});
	uint32_t minX = std::min(lowest.x, otherLowest.x);
	uint32_t minY = std::min(lowest.y, otherLowest.y);
	uint32_t minZ = std::min(lowest.z, otherLowest.z);
	output.m_lowest = m_blocks->getIndex({minX, minY, minZ});
	output.m_blocks = m_blocks;
	return output;
}
void Cuboid::merge(const Cuboid& cuboid)
{
	Cuboid sum = cuboid.sum(*this);
	m_highest = sum.m_highest;
	m_lowest = sum.m_lowest;
}
void Cuboid::setFrom(BlockIndex block)
{
	m_highest = m_lowest = block;
}
void Cuboid::setFrom(Blocks& blocks, BlockIndex a, BlockIndex b)
{
	Cuboid result = fromBlockPair(blocks, a, b);
	m_highest = result.m_highest;
	m_lowest = result.m_lowest;
	m_blocks = &blocks;
}
void Cuboid::clear() { m_lowest = m_highest = BLOCK_INDEX_MAX; m_blocks = nullptr;}
Cuboid Cuboid::getFace(Facing facing) const
{
	assert(facing < 6);
	Point3D highest = m_blocks->getCoordinates(m_highest);
	Point3D lowest = m_blocks->getCoordinates(m_lowest);
	// test area has x higher then this.
	if(facing == 4)
		return Cuboid(*m_blocks, m_highest, m_blocks->getIndex({highest.x, lowest.y, lowest.z}));
	// test area has x lower then this.
	else if(facing == 2)
		return Cuboid(*m_blocks, m_blocks->getIndex({lowest.x, highest.y, highest.z}), m_lowest);
	// test area has y higher then this.
	else if(facing == 3)
		return Cuboid(*m_blocks, m_highest, m_blocks->getIndex({lowest.x, highest.y, lowest.z}));
	// test area has y lower then this.
	else if(facing == 1)
		return Cuboid(*m_blocks, m_blocks->getIndex({highest.x, lowest.y, highest.z}), m_lowest);
			// test area has z higher then this.
	else if(facing == 5)
		return Cuboid(*m_blocks, m_highest, m_blocks->getIndex({lowest.x, lowest.y, highest.z}));
	// test area has z lower then this.
	else if(facing == 0)
		return Cuboid(*m_blocks, m_blocks->getIndex({highest.x, highest.y, lowest.z}), m_lowest);
	return Cuboid(*m_blocks, BLOCK_INDEX_MAX, BLOCK_INDEX_MAX);
}
bool Cuboid::overlapsWith(const Cuboid& other) const
{
	Point3D highest = m_blocks->getCoordinates(m_highest);
	Point3D lowest = m_blocks->getCoordinates(m_lowest);
	Point3D otherHighest = m_blocks->getCoordinates(other.m_highest);
	Point3D otherLowest = m_blocks->getCoordinates(other.m_lowest);
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
size_t Cuboid::size() const
{
	if(m_highest == BLOCK_INDEX_MAX)
	{
		assert(m_lowest == BLOCK_INDEX_MAX);
		return 0;
	}
	assert(m_lowest != BLOCK_INDEX_MAX);
	Point3D highest = m_blocks->getCoordinates(m_highest);
	Point3D lowest = m_blocks->getCoordinates(m_lowest);
	return ((highest.x + 1) - lowest.x) * ((highest.y + 1) - lowest.y) * ((highest.z + 1) - lowest.z);
}
// static method
Cuboid Cuboid::fromBlock(Blocks& blocks, BlockIndex block)
{
	return Cuboid(blocks, block, block);
}
Cuboid Cuboid::fromBlockPair(Blocks& blocks, BlockIndex a, BlockIndex b)
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
	output.m_blocks = &blocks;
	return output;
}
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
Cuboid::iterator::iterator(Blocks& blocks, BlockIndex lowest, BlockIndex highest) : m_blocks(blocks) 
{
	if(lowest == BLOCK_INDEX_MAX)
	{
		assert(highest == BLOCK_INDEX_MAX);
		setToEnd();
	}
	else 
	{
		m_current = m_start = m_blocks.getCoordinates(lowest);
		m_end = m_blocks.getCoordinates(highest);
	}
}
void Cuboid::iterator::setToEnd()
{
	// TODO: All these assignments arn't needed, just one would be fine.
	m_start.x = m_start.y = m_start.z = m_current.x = m_current.y = m_current.z = m_end.x = m_end.y = m_end.z = BLOCK_DISTANCE_MAX;
}
Cuboid::iterator& Cuboid::iterator::iterator::operator++() 
{
	if (m_current.x < m_end.x) {
		m_current.x++;
	} else if (m_current.y < m_end.y) {
		m_current.x = m_start.x;
		m_current.y++;
	} else if (m_current.z < m_end.z) {
		m_current.x = m_start.x;
		m_current.y = m_start.y;
		m_current.z++;
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
