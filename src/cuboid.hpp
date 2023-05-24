#pragma once
#include <cassert>
template<class Block>
BaseCuboid<Block>::BaseCuboid(Block* h, Block* l) : m_highest(h), m_lowest(l)
{
	assert(m_highest->m_x >= m_lowest->m_x);
	assert(m_highest->m_y >= m_lowest->m_y);
	assert(m_highest->m_z >= m_lowest->m_z);
}
template<class Block>
BaseCuboid<Block>::BaseCuboid(Block& h, Block& l) : m_highest(&h), m_lowest(&l)
{
	assert(m_highest->m_x >= m_lowest->m_x);
	assert(m_highest->m_y >= m_lowest->m_y);
	assert(m_highest->m_z >= m_lowest->m_z);
}
template<class Block>
bool BaseCuboid<Block>::contains(const Block& block) const
{
	return (
			block.m_x <= m_highest->m_x && block.m_x >= m_lowest->m_x &&
			block.m_y <= m_highest->m_y && block.m_y >= m_lowest->m_y &&
			block.m_z <= m_highest->m_z && block.m_z >= m_lowest->m_z
	       );
}
template<class Block>
bool BaseCuboid<Block>::canMerge(const BaseCuboid<Block>& cuboid) const
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
template<class Block>
BaseCuboid<Block> BaseCuboid<Block>::sum(const BaseCuboid<Block>& cuboid) const
{
	assert(canMerge(cuboid));
	BaseCuboid<Block> output;
	uint32_t maxX = std::max(m_highest->m_x, cuboid.m_highest->m_x);
	uint32_t maxY = std::max(m_highest->m_y, cuboid.m_highest->m_y);
	uint32_t maxZ = std::max(m_highest->m_z, cuboid.m_highest->m_z);
	output.m_highest = &m_highest->m_area->m_blocks[maxX][maxY][maxZ];
	uint32_t minX = std::min(m_lowest->m_x, cuboid.m_lowest->m_x);
	uint32_t minY = std::min(m_lowest->m_y, cuboid.m_lowest->m_y);
	uint32_t minZ = std::min(m_lowest->m_z, cuboid.m_lowest->m_z);
	output.m_lowest = &m_highest->m_area->m_blocks[minX][minY][minZ];
	return output;
}
template<class Block>
void BaseCuboid<Block>::merge(const BaseCuboid<Block>& cuboid)
{
	BaseCuboid<Block> sum = cuboid.sum(*this);
	m_highest = sum.m_highest;
	m_lowest = sum.m_lowest;
}
template<class Block>
BaseCuboid<Block> BaseCuboid<Block>::getFace(uint32_t facing) const
{
	assert(facing < 6);
	// test area has x higher then this.
	if(facing == 4)
		return BaseCuboid<Block>(m_highest, &m_highest->m_area->m_blocks[m_highest->m_x][m_lowest->m_y][m_lowest->m_z]);
	// test area has x lower then this.
	else if(facing == 2)
		return BaseCuboid<Block>(&m_highest->m_area->m_blocks[m_lowest->m_x][m_highest->m_y][m_highest->m_z], m_lowest);
	// test area has y higher then this.
	else if(facing == 3)
		return BaseCuboid<Block>(m_highest, &m_highest->m_area->m_blocks[m_lowest->m_x][m_highest->m_y][m_lowest->m_z]);
	// test area has y lower then this.
	else if(facing == 1)
		return BaseCuboid<Block>(&m_highest->m_area->m_blocks[m_highest->m_x][m_lowest->m_y][m_highest->m_z], m_lowest);
			// test area has z higher then this.
	else if(facing == 5)
		return BaseCuboid<Block>(m_highest, &m_highest->m_area->m_blocks[m_lowest->m_x][m_lowest->m_y][m_highest->m_z]);
	// test area has z lower then this.
	else if(facing == 0)
		return BaseCuboid<Block>(&m_highest->m_area->m_blocks[m_highest->m_x][m_highest->m_y][m_lowest->m_z], m_lowest);
	return BaseCuboid<Block>(nullptr, nullptr);
}
template<class Block>
bool BaseCuboid<Block>::overlapsWith(const BaseCuboid<Block>& cuboid) const
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
template<class Block>
size_t BaseCuboid<Block>::size() const
{
	return ((m_highest->m_x + 1) - m_lowest->m_x) * ((m_highest->m_y + 1) - m_lowest->m_y) * ((m_highest->m_z + 1) - m_lowest->m_z);
}
template<class Block>
bool BaseCuboid<Block>::operator==(const BaseCuboid<Block>& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
template<class Block>
BaseCuboid<Block>::iterator::iterator(BaseCuboid<Block>& c, const Block& block) : cuboid(&c), x(block.m_x), y(block.m_y), z(block.m_z) {}
template<class Block>
BaseCuboid<Block>::iterator::iterator() : cuboid(nullptr), x(0), y(0), z(0) {}
template<class Block>
BaseCuboid<Block>::iterator& BaseCuboid<Block>::iterator::operator++()
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
template<class Block>
BaseCuboid<Block>::iterator BaseCuboid<Block>::iterator::operator++(int) const
{
	BaseCuboid<Block>::iterator output = *this;
	++output;
	return output;
}
template<class Block>
bool BaseCuboid<Block>::iterator::operator==(const BaseCuboid<Block>::iterator other) const { return other.x == x && other.y == y && other.z == z; }
template<class Block>
bool BaseCuboid<Block>::iterator::operator!=(const BaseCuboid<Block>::iterator other) const { return other.x != x || other.y != y || other.z != z; }
template<class Block>
Block& BaseCuboid<Block>::iterator::operator*() const { return cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class Block>
Block* BaseCuboid<Block>::iterator::operator->() const { return &cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class Block>
BaseCuboid<Block>::iterator BaseCuboid<Block>::begin(){ return BaseCuboid<Block>::iterator(*this, *m_lowest); }
template<class Block>
BaseCuboid<Block>::iterator BaseCuboid<Block>::end()
{
	BaseCuboid<Block>::iterator iterator(*this, *m_highest);
	return ++iterator;
}
template<class Block>
BaseCuboid<Block>::const_iterator::const_iterator(const BaseCuboid<Block>& c, const Block& block) : cuboid(&c), x(block.m_x), y(block.m_y), z(block.m_z) {}
template<class Block>
BaseCuboid<Block>::const_iterator::const_iterator() : cuboid(nullptr), x(0), y(0), z(0) {}
template<class Block>
BaseCuboid<Block>::const_iterator& BaseCuboid<Block>::const_iterator::operator++()
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
template<class Block>
BaseCuboid<Block>::const_iterator BaseCuboid<Block>::const_iterator::operator++(int) const
{
	BaseCuboid<Block>::const_iterator output = *this;
	return ++output;
}
template<class Block>
bool BaseCuboid<Block>::const_iterator::operator==(const BaseCuboid<Block>::const_iterator other) const { return other.x == x && other.y == y && other.z == z; }
template<class Block>
bool BaseCuboid<Block>::const_iterator::operator!=(const BaseCuboid<Block>::const_iterator other) const { return other.x != x || other.y != y || other.z != z; }
template<class Block>
const Block& BaseCuboid<Block>::const_iterator::operator*() const { return cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class Block>
const Block* BaseCuboid<Block>::const_iterator::operator->() const { return &cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class Block>
BaseCuboid<Block>::const_iterator BaseCuboid<Block>::begin() const { return BaseCuboid<Block>::const_iterator(*this, *m_lowest); }
template<class Block>
BaseCuboid<Block>::const_iterator BaseCuboid<Block>::end() const
{
	BaseCuboid<Block>::const_iterator iterator(*this, *m_highest);
	return ++iterator;
}
