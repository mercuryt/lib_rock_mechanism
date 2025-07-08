#include "cuboidSet.h"
#include "offsetCuboidSet.h"
CuboidSet::CuboidSet(const Point3D& location, const Facing4& rotation, const OffsetCuboidSet& offsetPairs)
{
	const Point3D coordinates = location;
	auto end = offsetPairs.cuboidsEnd();
	for(auto iter = offsetPairs.cuboidsBegin(); iter < end; ++iter)
	{
		auto [high, low] = iter->toOffsetPair();
		high += coordinates;
		low += coordinates;
		high.rotate2D(rotation);
		low.rotate2D(rotation);
		add({Point3D::create(high), Point3D::create(low)});
	}
}
void CuboidSet::create(const Cuboid& cuboid)
{
	uint i = 0;
	for(const Cuboid& existing : m_cuboids)
	{
		assert(!cuboid.intersects(existing));
		if(existing.isTouching(cuboid) && existing.canMerge(cuboid))
		{
			mergeInternal(cuboid, i);
			return;
		}
		++i;
	}
	m_cuboids.insert(cuboid);
}
void CuboidSet::destroy(const uint& index)
{
	m_cuboids.eraseIndex(index);
}
void CuboidSet::add(const Point3D& point)
{
	add({point, point});
}
void CuboidSet::remove(const Point3D& point)
{
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
	SmallSet<std::pair<Cuboid, uint>> toSplit;
	uint i = 0;
	for(const Cuboid& existing : m_cuboids)
	{
		if(existing.intersects(cuboid))
			toSplit.emplace(existing, i);
		++i;
	}
	for(const auto& pair : toSplit)
		destroy(pair.second);
	for(const auto& pair : toSplit)
		for(const Cuboid& splitResult : pair.first.getChildrenWhenSplitByCuboid(cuboid))
			create(splitResult);
}
void CuboidSet::shift(const Offset3D offset, const Distance& distance)
{
	for(Cuboid& cuboid : m_cuboids)
		cuboid.shift(offset, distance);
}
void CuboidSet::addSet(const CuboidSet& other)
{
	for(const Cuboid& cuboid : other.getCuboids())
		add(cuboid);
}
void CuboidSet::rotateAroundPoint(const Point3D& point, const Facing4& rotation)
{
	for(Cuboid& cuboid : m_cuboids)
		cuboid.rotateAroundPoint(point, rotation);
}
void CuboidSet::mergeInternal(const Cuboid& absorbed, const uint& absorber)
{
	const Cuboid absorberCopy = m_cuboids[absorber];
	assert(absorbed.canMerge(absorberCopy));
	assert(absorberCopy.canMerge(absorbed));
	for(const Cuboid& existing : m_cuboids)
		if(existing != absorbed)
			assert(!existing.intersects(absorbed));
	Cuboid newCuboid = absorberCopy.sum(absorbed);
	destroy(absorber);
	// Call create may trigger another merge.
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
bool CuboidSet::contains(const Point3D& point) const
{
	for(const Cuboid& cuboid : m_cuboids)
		if(cuboid.contains(point))
			return true;
	return false;
}
SmallSet<Point3D> CuboidSet::toPointSet() const
{
	SmallSet<Point3D> output;
	for(const Cuboid& cuboid : m_cuboids)
		output.maybeInsertAll(cuboid);
	return output;
}
bool CuboidSet::isAdjacent(const Cuboid& cuboid) const
{
	for(const auto& c : m_cuboids)
		if(c.isTouching(cuboid))
			return true;
	return false;
}
CuboidSet CuboidSet::create(const SmallSet<Point3D>& points)
{
	CuboidSet output;
	for(const Point3D& point : points)
		output.add({point, point});
	return output;
}
void to_json(Json& data, const CuboidSet& cuboidSet)
{
	for(const Cuboid& cuboid : cuboidSet.getCuboids())
		data.push_back({cuboid.m_highest, cuboid.m_lowest});
}
void from_json(const Json& data, CuboidSet& cuboidSet)
{
	for(const Json& pair : data)
	{
		Cuboid cuboid(pair[0].get<Point3D>(), pair[1].get<Point3D>());
		cuboidSet.add(cuboid);
	}
}
CuboidSetConstIterator::CuboidSetConstIterator(const CuboidSet& cuboidSet, bool end) :
	m_cuboidSet(cuboidSet),
	m_outerIter(end ? m_cuboidSet.m_cuboids.end() : cuboidSet.m_cuboids.begin()),
	m_innerIter(end ? m_cuboidSet.m_cuboids.end()->end() : m_outerIter->begin())
{ }
CuboidSetConstIterator& CuboidSetConstIterator::operator++()
{
	++m_innerIter;
	if(m_innerIter == m_outerIter->end())
	{
		++m_outerIter;
		if(m_outerIter != m_cuboidSet.m_cuboids.end())
			m_innerIter = m_outerIter->begin();
	}
	return *this;
}
CuboidSetConstIterator CuboidSetConstIterator::operator++(int)
{
	auto output = *this;
	++(*this);
	return output;
}
SmallSet<Cuboid> CuboidSetWithBoundingBoxAdjacent::removeAndReturnNoLongerAdjacentCuboids(const Point3D& point) { return removeAndReturnNoLongerAdjacentCuboids({point, point}); }
SmallSet<Cuboid> CuboidSetWithBoundingBoxAdjacent::removeAndReturnNoLongerAdjacentCuboids(const Cuboid& cuboid)
{
	bool changesBoundingBox = cuboid.contains(m_boundingBox.m_highest) || cuboid.contains(m_boundingBox.m_lowest);
	SmallSet<std::pair<Cuboid, uint>> toSplit;
	SmallSet<Cuboid> newlySplit;
	uint i = 0;
	for(const Cuboid& existing : m_cuboids)
	{
		if(existing.intersects(cuboid))
			toSplit.emplace(existing, i);
		++i;
	}
	for(const auto& pair : toSplit)
		destroy(pair.second);
	for(const auto& pair : toSplit)
		for(const Cuboid& splitResult : pair.first.getChildrenWhenSplitByCuboid(cuboid))
			if(CuboidSet::isAdjacent(splitResult))
				create(splitResult);
			else
				newlySplit.insert(splitResult);
	if(changesBoundingBox)
	{
		m_boundingBox.clear();
		for(const Cuboid& otherCuboid : getCuboids())
			m_boundingBox.maybeExpand(otherCuboid);
	}
	return newlySplit;
}
void CuboidSetWithBoundingBoxAdjacent::addAndExtend(const Point3D& point) { return addAndExtend({point, point}); }
void CuboidSetWithBoundingBoxAdjacent::addAndExtend(const Cuboid& cuboid)
{
	assert(CuboidSet::isAdjacent(cuboid));
	CuboidSet::add(cuboid);
	m_boundingBox.maybeExpand(cuboid);
}
bool CuboidSetWithBoundingBoxAdjacent::isAdjacent(const CuboidSetWithBoundingBoxAdjacent& cuboidSet) const
{
	// If the bounding boxes don't touch then no need to check the actual cuboids.
	if(!cuboidSet.getBoundingBox().intersects(getBoundingBox()))
		return false;
	// Check each cubid in this set against each in the other set.
	for(const auto& cuboid : m_cuboids)
		for(const auto& otherCuboid : cuboidSet.m_cuboids)
			if(cuboid.isTouching(otherCuboid))
				return true;
	return false;
}