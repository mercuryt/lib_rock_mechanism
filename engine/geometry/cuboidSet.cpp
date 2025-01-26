#include "cuboidSet.h"
#include "../blocks/blocks.h"
void CuboidSet::create(const Cuboid& cuboid)
{
	m_cuboids.insert(cuboid);
	for(const Cuboid& existing : m_cuboids)
	{
		assert(!cuboid.overlapsWith(existing));
		if(existing.canMerge(cuboid))
		{
			merge(cuboid, existing);
			return;
		}
	}
}
void CuboidSet::add(const Point3D& point)
{
	add({point, point});
}
void CuboidSet::remove(const Point3D& point)
{
	remove({point, point});
}
void CuboidSet::add(const Blocks& blocks, const BlockIndex& block)
{
	const Point3D& point = blocks.getCoordinates(block);
	add({point, point});
}
void CuboidSet::remove(const Blocks& blocks, const BlockIndex& block)
{
	const Point3D& point = blocks.getCoordinates(block);
	remove({point, point});
}
void CuboidSet::add(const Cuboid& cuboid)
{
	remove(cuboid);
	create(cuboid);
}
void CuboidSet::remove(const Cuboid& cuboid)
{
	//TODO: partition instead of toSplit.
	SmallSet<Cuboid> toSplit;
	for(const Cuboid& existing : m_cuboids)
		if(existing.overlapsWith(cuboid))
			toSplit.insert(existing);
	for(const Cuboid& toSplitCuboid : toSplit)
		m_cuboids.erase(toSplitCuboid);
	for(const Cuboid& toSplitCuboid : toSplit)
		for(const Cuboid& splitResult : toSplitCuboid.getChildrenWhenSplitByCuboid(cuboid))
			create(splitResult);
}
void CuboidSet::merge(const Cuboid& absorbed, const Cuboid& absorber)
{
	for(const Cuboid& existing : m_cuboids)
		assert(existing.overlapsWith(absorbed));
	for(const Cuboid& existing : m_cuboids)
		assert(existing.overlapsWith(absorber));
	m_cuboids.maybeErase(absorbed);
	m_cuboids.erase(absorber);
	Cuboid newCuboid = absorber.sum(absorbed);
	// Call create to potentailly trigger another merge.
	create(newCuboid);
}
bool CuboidSet::empty() const
{
	return m_cuboids.empty();
}
uint CuboidSet::size() const
{
	uint output = 0;
	for(const Cuboid& cuboid : m_cuboids)
		output += cuboid.size();
	return output;
}
bool CuboidSet::contains(const Blocks& blocks, const BlockIndex& block) const
{
	return contains(blocks.getCoordinates(block));
}
bool CuboidSet::contains(const Point3D& point) const
{
	for(const Cuboid& cuboid : m_cuboids)
		if(cuboid.contains(point))
			return true;
	return false;
}
SmallSet<BlockIndex> CuboidSet::toBlockSet(const Blocks& blocks) const
{
	SmallSet<BlockIndex> output;
	for(const Cuboid& cuboid : m_cuboids)
	{
		const auto view = cuboid.getView(blocks);
		output.maybeInsertAll(view.begin(), view.end());
	}
	return output;
}
CuboidSetConstIterator::CuboidSetConstIterator(const Blocks& blocks, const CuboidSet& cuboidSet, bool end) :
	m_blocks(blocks),
	m_cuboidSet(cuboidSet),
	m_outerIter(end ? m_cuboidSet.m_cuboids.end() : cuboidSet.m_cuboids.begin()),
	m_innerIter(end ? m_cuboidSet.m_cuboids.begin()->begin(blocks) : m_outerIter->begin(m_blocks))
{ }
CuboidSetConstIterator& CuboidSetConstIterator::operator++()
{
	++m_innerIter;
	if(m_innerIter == m_outerIter->end(m_blocks))
	{
		++m_outerIter;
		if(m_outerIter != m_cuboidSet.m_cuboids.end())
			m_innerIter = m_outerIter->begin(m_blocks);
	}
	return *this;
}
CuboidSetConstIterator CuboidSetConstIterator::operator++(int)
{
	auto output = *this;
	++(*this);
	return output;
}
void CuboidSetWithBoundingBox::remove(const Cuboid& cuboid)
{
	bool changesBoundingBox = cuboid.contains(m_boundingBox.m_highest) || cuboid.contains(m_boundingBox.m_lowest);
	CuboidSet::remove(cuboid);
	if(changesBoundingBox)
	{
		m_boundingBox.clear();
		for(const Cuboid& otherCuboid : getCuboids())
			m_boundingBox.maybeExpand(otherCuboid);
	}
}
void CuboidSetWithBoundingBox::add(const Cuboid& cuboid)
{
	CuboidSet::add(cuboid);
	m_boundingBox.maybeExpand(cuboid);
}