#pragma once
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::Cuboid(DerivedBlock* h, DerivedBlock* l) : m_highest(h), m_lowest(l)
{
	assert(m_highest->m_x >= m_lowest->m_x);
	assert(m_highest->m_y >= m_lowest->m_y);
	assert(m_highest->m_z >= m_lowest->m_z);
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::Cuboid(DerivedBlock& h, DerivedBlock& l) : m_highest(&h), m_lowest(&l)
{
	assert(m_highest->m_x >= m_lowest->m_x);
	assert(m_highest->m_y >= m_lowest->m_y);
	assert(m_highest->m_z >= m_lowest->m_z);
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::contains(const DerivedBlock& block) const
{
	return (
			block.m_x <= m_highest->m_x && block.m_x >= m_lowest->m_x &&
			block.m_y <= m_highest->m_y && block.m_y >= m_lowest->m_y &&
			block.m_z <= m_highest->m_z && block.m_z >= m_lowest->m_z
	       );
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::canMerge(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>& cuboid) const
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
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea> Cuboid<DerivedBlock, DerivedActor, DerivedArea>::sum(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>& cuboid) const
{
	assert(canMerge(cuboid));
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> output;
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
template<class DerivedBlock, class DerivedActor, class DerivedArea>
void Cuboid<DerivedBlock, DerivedActor, DerivedArea>::merge(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>& cuboid)
{
	Cuboid<DerivedBlock, DerivedActor, DerivedArea> sum = cuboid.sum(*this);
	m_highest = sum.m_highest;
	m_lowest = sum.m_lowest;
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea> Cuboid<DerivedBlock, DerivedActor, DerivedArea>::getFace(uint32_t facing) const
{
	assert(facing < 6);
	// test area has x higher then this.
	if(facing == 4)
		return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(m_highest, &m_highest->m_area->m_blocks[m_highest->m_x][m_lowest->m_y][m_lowest->m_z]);
	// test area has x lower then this.
	else if(facing == 2)
		return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(&m_highest->m_area->m_blocks[m_lowest->m_x][m_highest->m_y][m_highest->m_z], m_lowest);
	// test area has y higher then this.
	else if(facing == 3)
		return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(m_highest, &m_highest->m_area->m_blocks[m_lowest->m_x][m_highest->m_y][m_lowest->m_z]);
	// test area has y lower then this.
	else if(facing == 1)
		return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(&m_highest->m_area->m_blocks[m_highest->m_x][m_lowest->m_y][m_highest->m_z], m_lowest);
			// test area has z higher then this.
	else if(facing == 5)
		return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(m_highest, &m_highest->m_area->m_blocks[m_lowest->m_x][m_lowest->m_y][m_highest->m_z]);
	// test area has z lower then this.
	else if(facing == 0)
		return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(&m_highest->m_area->m_blocks[m_highest->m_x][m_highest->m_y][m_lowest->m_z], m_lowest);
	return Cuboid<DerivedBlock, DerivedActor, DerivedArea>(nullptr, nullptr);
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::overlapsWith(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>& cuboid) const
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
template<class DerivedBlock, class DerivedActor, class DerivedArea>
size_t Cuboid<DerivedBlock, DerivedActor, DerivedArea>::size() const
{
	return ((m_highest->m_x + 1) - m_lowest->m_x) * ((m_highest->m_y + 1) - m_lowest->m_y) * ((m_highest->m_z + 1) - m_lowest->m_z);
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::operator==(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>& cuboid) const
{
	return m_lowest == cuboid.m_lowest && m_highest == cuboid.m_highest;
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::iterator(Cuboid<DerivedBlock, DerivedActor, DerivedArea>& c, const DerivedBlock& block) : cuboid(&c), x(block.m_x), y(block.m_y), z(block.m_z) {}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::iterator() : cuboid(nullptr), x(0), y(0), z(0) {}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator& Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::operator++()
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
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::operator++(int) const
{
	Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator output = *this;
	++output;
	return output;
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::operator==(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator other) const { return other.x == x && other.y == y && other.z == z; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::operator!=(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator other) const { return other.x != x || other.y != y || other.z != z; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
DerivedBlock& Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::operator*() const { return cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
DerivedBlock* Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator::operator->() const { return &cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator Cuboid<DerivedBlock, DerivedActor, DerivedArea>::begin(){ return Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator(*this, *m_lowest); }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator Cuboid<DerivedBlock, DerivedActor, DerivedArea>::end()
{
	Cuboid<DerivedBlock, DerivedActor, DerivedArea>::iterator iterator(*this, *m_highest);
	return ++iterator;
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::const_iterator(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>& c, const DerivedBlock& block) : cuboid(&c), x(block.m_x), y(block.m_y), z(block.m_z) {}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::const_iterator() : cuboid(nullptr), x(0), y(0), z(0) {}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator& Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::operator++()
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
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::operator++(int) const
{
	Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator output = *this;
	return ++output;
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::operator==(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator other) const { return other.x == x && other.y == y && other.z == z; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::operator!=(const Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator other) const { return other.x != x || other.y != y || other.z != z; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
const DerivedBlock& Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::operator*() const { return cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
const DerivedBlock* Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator::operator->() const { return &cuboid->m_highest->m_area->m_blocks[x][y][z]; }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator Cuboid<DerivedBlock, DerivedActor, DerivedArea>::begin() const { return Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator(*this, *m_lowest); }
template<class DerivedBlock, class DerivedActor, class DerivedArea>
Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator Cuboid<DerivedBlock, DerivedActor, DerivedArea>::end() const
{
	Cuboid<DerivedBlock, DerivedActor, DerivedArea>::const_iterator iterator(*this, *m_highest);
	return ++iterator;
}
