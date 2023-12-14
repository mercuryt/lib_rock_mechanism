#include "cuboid.h"
#include "block.h"
#include "area.h"

#include <cassert>
Cuboid::Cuboid(Block* h, Block* l) : m_highest(h), m_lowest(l)
{
	assert(m_highest->m_x >= m_lowest->m_x);
	assert(m_highest->m_y >= m_lowest->m_y);
	assert(m_highest->m_z >= m_lowest->m_z);
}
Cuboid::Cuboid(Block& h, Block& l) : m_highest(&h), m_lowest(&l)
{
	assert(m_highest->m_x >= m_lowest->m_x);
	assert(m_highest->m_y >= m_lowest->m_y);
	assert(m_highest->m_z >= m_lowest->m_z);
}
std::unordered_set<Block*> Cuboid::toSet()
{
	std::unordered_set<Block*> output;
	for(Block& block : *this)
		output.insert(&block);
	return output;
}
bool Cuboid::contains(const Block& block) const
{
	return (
			block.m_x <= m_highest->m_x && block.m_x >= m_lowest->m_x &&
			block.m_y <= m_highest->m_y && block.m_y >= m_lowest->m_y &&
			block.m_z <= m_highest->m_z && block.m_z >= m_lowest->m_z
	       );
}
bool Cuboid::canMerge(const Cuboid& cuboid) const
{
	uint32_t count = 0;
	if(cuboid.m_highest->m_x == m_highest->m_x && cuboid.m_lowest->m_x == m_lowest->m_x)
		count++;
	if(cuboid.m_highest->m_y == m_highest->m_y && cuboid.m_lowest->m_y == m_lowest->m_y)
		count++;
	if(cuboid.m_highest->m_z == m_highest->m_z && cuboid.m_lowest->m_z == m_lowest->m_z)
		count++;
	assert(count != 3);
	return count == 2;
}
Cuboid Cuboid::sum(const Cuboid& cuboid) const
{
	assert(canMerge(cuboid));
	Cuboid output;
	uint32_t maxX = std::max(m_highest->m_x, cuboid.m_highest->m_x);
	uint32_t maxY = std::max(m_highest->m_y, cuboid.m_highest->m_y);
	uint32_t maxZ = std::max(m_highest->m_z, cuboid.m_highest->m_z);
	output.m_highest = &m_highest->m_area->getBlock(maxX, maxY, maxZ);
	uint32_t minX = std::min(m_lowest->m_x, cuboid.m_lowest->m_x);
	uint32_t minY = std::min(m_lowest->m_y, cuboid.m_lowest->m_y);
	uint32_t minZ = std::min(m_lowest->m_z, cuboid.m_lowest->m_z);
	output.m_lowest = &m_highest->m_area->getBlock(minX, minY, minZ);
	return output;
}
void Cuboid::merge(const Cuboid& cuboid)
{
	Cuboid sum = cuboid.sum(*this);
	m_highest = sum.m_highest;
	m_lowest = sum.m_lowest;
}
Cuboid Cuboid::getFace(uint32_t facing) const
{
	assert(facing < 6);
	// test area has x higher then this.
	if(facing == 4)
		return Cuboid(m_highest, &m_highest->m_area->getBlock(m_highest->m_x, m_lowest->m_y, m_lowest->m_z));
	// test area has x lower then this.
	else if(facing == 2)
		return Cuboid(&m_highest->m_area->getBlock(m_lowest->m_x, m_highest->m_y, m_highest->m_z), m_lowest);
	// test area has y higher then this.
	else if(facing == 3)
		return Cuboid(m_highest, &m_highest->m_area->getBlock(m_lowest->m_x, m_highest->m_y, m_lowest->m_z));
	// test area has y lower then this.
	else if(facing == 1)
		return Cuboid(&m_highest->m_area->getBlock(m_highest->m_x, m_lowest->m_y, m_highest->m_z), m_lowest);
			// test area has z higher then this.
	else if(facing == 5)
		return Cuboid(m_highest, &m_highest->m_area->getBlock(m_lowest->m_x, m_lowest->m_y, m_highest->m_z));
	// test area has z lower then this.
	else if(facing == 0)
		return Cuboid(&m_highest->m_area->getBlock(m_highest->m_x, m_highest->m_y, m_lowest->m_z), m_lowest);
	return Cuboid(nullptr, nullptr);
}
bool Cuboid::overlapsWith(const Cuboid& cuboid) const
{
	return
			(
			 m_highest->m_x >= cuboid.m_lowest->m_x && m_highest->m_x <= cuboid.m_highest->m_x &&
			 m_highest->m_y >= cuboid.m_lowest->m_y && m_highest->m_y <= cuboid.m_highest->m_y &&
			 m_highest->m_z >= cuboid.m_lowest->m_z && m_highest->m_z <= cuboid.m_highest->m_z
			) ||
			(
			 cuboid.m_highest->m_x >= m_lowest->m_x && cuboid.m_highest->m_x <= m_highest->m_x &&
			 cuboid.m_highest->m_y >= m_lowest->m_y && cuboid.m_highest->m_y <= m_highest->m_y &&
			 cuboid.m_highest->m_z >= m_lowest->m_z && cuboid.m_highest->m_z <= m_highest->m_z
			) ||
			(
			 m_lowest->m_x >= cuboid.m_lowest->m_x && m_lowest->m_x <= cuboid.m_highest->m_x &&
			 m_lowest->m_y >= cuboid.m_lowest->m_y && m_lowest->m_y <= cuboid.m_highest->m_y &&
			 m_lowest->m_z >= cuboid.m_lowest->m_z && m_lowest->m_z <= cuboid.m_highest->m_z
			) ||
			(
			 cuboid.m_lowest->m_x >= m_lowest->m_x && cuboid.m_lowest->m_x <= m_highest->m_x &&
			 cuboid.m_lowest->m_y >= m_lowest->m_y && cuboid.m_lowest->m_y <= m_highest->m_y &&
			 cuboid.m_lowest->m_z >= m_lowest->m_z && cuboid.m_lowest->m_z <= m_highest->m_z
			);
}
size_t Cuboid::size() const
{
	return ((m_highest->m_x + 1) - m_lowest->m_x) * ((m_highest->m_y + 1) - m_lowest->m_y) * ((m_highest->m_z + 1) - m_lowest->m_z);
}
bool Cuboid::operator==(const Cuboid& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
Cuboid::iterator::iterator(Cuboid& c, const Block& block) : cuboid(&c), x(block.m_x), y(block.m_y), z(block.m_z) {}
Cuboid::iterator::iterator() : cuboid(nullptr), x(0), y(0), z(0) {}
Cuboid::iterator& Cuboid::iterator::operator++()
{
	if(++z > cuboid->m_highest->m_z)
	{
		z = cuboid->m_lowest->m_z;
		if(++y > cuboid->m_highest->m_y)
		{
			y = cuboid->m_lowest->m_y;
			++x;
		}
	}
	return *this;
}
Cuboid::iterator Cuboid::iterator::operator++(int) const
{
	Cuboid::iterator output = *this;
	++output;
	return output;
}
bool Cuboid::iterator::operator==(const Cuboid::iterator other) const { return other.x == x && other.y == y && other.z == z; }
bool Cuboid::iterator::operator!=(const Cuboid::iterator other) const { return other.x != x || other.y != y || other.z != z; }
Block& Cuboid::iterator::operator*() const { return cuboid->m_highest->m_area->getBlock(x, y, z); }
Block* Cuboid::iterator::operator->() const { return &cuboid->m_highest->m_area->getBlock(x, y, z); }
Cuboid::iterator Cuboid::begin(){ return Cuboid::iterator(*this, *m_lowest); }
Cuboid::iterator Cuboid::end()
{
	Cuboid::iterator iterator(*this, *m_highest);
	return ++iterator;
}
Cuboid::const_iterator::const_iterator(const Cuboid& c, const Block& block) : cuboid(&c), x(block.m_x), y(block.m_y), z(block.m_z) {}
Cuboid::const_iterator::const_iterator() : cuboid(nullptr), x(0), y(0), z(0) {}
Cuboid::const_iterator& Cuboid::const_iterator::operator++()
{
	if(++z > cuboid->m_highest->m_z)
	{
		z = cuboid->m_lowest->m_z;
		if(++y > cuboid->m_highest->m_y)
		{
			y = cuboid->m_lowest->m_y;
			++x;
		}
	}
	return *this;
}
Cuboid::const_iterator Cuboid::const_iterator::operator++(int) const
{
	Cuboid::const_iterator output = *this;
	return ++output;
}
bool Cuboid::const_iterator::operator==(const Cuboid::const_iterator other) const { return other.x == x && other.y == y && other.z == z; }
bool Cuboid::const_iterator::operator!=(const Cuboid::const_iterator other) const { return other.x != x || other.y != y || other.z != z; }
const Block& Cuboid::const_iterator::operator*() const { return cuboid->m_highest->m_area->getBlock(x, y, z); }
const Block* Cuboid::const_iterator::operator->() const { return &cuboid->m_highest->m_area->getBlock(x, y, z); }
Cuboid::const_iterator Cuboid::begin() const { return Cuboid::const_iterator(*this, *m_lowest); }
Cuboid::const_iterator Cuboid::end() const
{
	Cuboid::const_iterator iterator(*this, *m_highest);
	return ++iterator;
}
