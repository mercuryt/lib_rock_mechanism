#include "cuboidSet.h"
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetBase<CuboidType, PointType, CuboidSetType>::CuboidSetBase(const PointType& location, const Facing4& rotation, const OffsetCuboidSet& offsetPairs)
{
	const PointType coordinates = location;
	auto end = offsetPairs.end();
	for(auto iter = offsetPairs.begin(); iter < end; ++iter)
	{
		auto [high, low] = iter->toOffsetPair();
		high += coordinates;
		low += coordinates;
		high.rotate2D(rotation);
		low.rotate2D(rotation);
		if constexpr(std::is_same_v<PointType, Offset3D>)
			add(CuboidType{high, low});
		else
			add(CuboidType{PointType::create(high), PointType::create(low)});
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetBase<CuboidType, PointType, CuboidSetType>::CuboidSetBase(const std::initializer_list<CuboidType>& cuboids)
{
	for(const CuboidType& cuboid : cuboids)
		m_cuboids.insert(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::insertOrMerge(const CuboidType& cuboid)
{
	uint i = 0;
	for(const CuboidType& existing : m_cuboids)
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
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::destroy(const uint& index)
{
	m_cuboids.eraseIndex(index);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::add(const PointType& point)
{
	add({point, point});
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::addAll(const CuboidSetType& other)
{
	if(empty())
		for(const CuboidType& cuboid : other)
			m_cuboids.insert(cuboid);
	else
		for(const CuboidType& cuboid : other)
			add(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::remove(const PointType& point)
{
	remove({point, point});
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::removeContainedAndFragmentIntercepted(const CuboidType& cuboid)
{
	const auto copy = m_cuboids;
	m_cuboids.clear();
	for(auto& existingCuboid : copy)
	{
		if(!cuboid.intersects(existingCuboid))
			m_cuboids.insert(existingCuboid);
		else if(cuboid.contains(existingCuboid))
			continue;
		else
			for(const CuboidType& fragment : existingCuboid.getChildrenWhenSplitBy(cuboid))
				m_cuboids.insert(fragment);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::removeContainedAndFragmentInterceptedAll(const CuboidSetType& cuboids)
{
	for(const CuboidType& cuboid : cuboids)
		removeContainedAndFragmentIntercepted(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::add(const CuboidType& cuboid)
{
	remove(cuboid);
	insertOrMerge(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::remove(const CuboidType& cuboid)
{
	assert(cuboid.exists());
	//TODO: partition instead of toSplit.
	SmallSet<std::pair<CuboidType, uint>> toSplit;
	uint i = 0;
	for(const CuboidType& existing : m_cuboids)
	{
		if(existing.intersects(cuboid))
			toSplit.emplace(existing, i);
		++i;
	}
	SmallSet<CuboidType> copy;
	copy.reserve(m_cuboids.size() - toSplit.size());
	const uint count = m_cuboids.size();
	for(i = 0; i != count; ++i)
	{
		const auto iter = std::ranges::find(toSplit.m_data, i, &std::pair<CuboidType, uint>::second);
		if(iter == toSplit.m_data.end())
			copy.insert(m_cuboids[i]);
	}
	m_cuboids = std::move(copy);
	for(const auto& pair : toSplit)
		for(const CuboidType& splitResult : pair.first.getChildrenWhenSplitByCuboid(cuboid))
			insertOrMerge(splitResult);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::shift(const Offset3D offset, const Distance& distance)
{
	for(CuboidType& cuboid : m_cuboids)
		cuboid.shift(offset, distance);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::addSet(const CuboidSetType& other)
{
	for(const CuboidType& cuboid : other.getCuboids())
		add(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::rotateAroundPoint(const PointType& point, const Facing4& rotation)
{
	for(CuboidType& cuboid : m_cuboids)
		cuboid.rotateAroundPoint(point, rotation);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::swap(CuboidSetType& other) { other.m_cuboids.swap(m_cuboids); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::popBack() { m_cuboids.popBack(); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::mergeInternal(const CuboidType& absorbed, const uint& absorber)
{
	CuboidType absorberCopy = m_cuboids[absorber];
	assert(absorbed.canMerge(absorberCopy));
	assert(absorberCopy.canMerge(absorbed));
	for(const CuboidType& existing : m_cuboids)
		if(existing != absorbed)
			assert(!existing.intersects(absorbed));
	absorberCopy.maybeExpand(absorbed);
	destroy(absorber);
	// Call create may trigger another merge.
	insertOrMerge(absorberCopy);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType CuboidSetBase<CuboidType, PointType, CuboidSetType>::center() const
{
	Offset3D sum = Offset3D::create(0,0,0);
	uint totalVolume = 0;
	for(const CuboidType& cuboid : m_cuboids)
	{
		totalVolume += cuboid.volume();
		for(const PointType& point : cuboid)
			if constexpr(std::is_same_v<PointType, Offset3D>)
				sum += point;
			else
				sum += point.toOffset();
	}
	sum.data /= totalVolume;
	if constexpr(std::is_same_v<PointType, Offset3D>)
		return sum;
	else
		return PointType::create(sum);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType::DimensionType CuboidSetBase<CuboidType, PointType, CuboidSetType>::lowestZ() const
{
	const CuboidType& lowest = std::ranges::min(m_cuboids.m_data, {}, [&](const CuboidType& cuboid) { return cuboid.m_low.z(); });
	return lowest.m_low.z();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::empty() const { return m_cuboids.empty(); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::exists() const { return !m_cuboids.empty(); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
uint CuboidSetBase<CuboidType, PointType, CuboidSetType>::size() const
{
	return m_cuboids.size();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
uint CuboidSetBase<CuboidType, PointType, CuboidSetType>::volume() const
{
	uint output = 0;
	for(const CuboidType& cuboid : m_cuboids)
		output += cuboid.volume();
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const Point3D& point) const
{
	for(const CuboidType& cuboid : m_cuboids)
		if(cuboid.contains(point))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const Offset3D& point) const
{
	for(const CuboidType& cuboid : m_cuboids)
		if(cuboid.contains(point))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const Cuboid& cuboid) const
{
	CuboidSetType remainder;
	if constexpr(std::is_same_v<PointType, Offset3D>)
		remainder.add(OffsetCuboid::create(cuboid));
	else
		remainder.add(cuboid);
	for(const CuboidType& other : m_cuboids)
	{
		auto remainderCopy = remainder.m_cuboids;
		for(const CuboidType& remainderCuboid : remainderCopy)
		{
			if(!remainderCuboid.intersects(other))
				continue;
			if(other.contains(remainderCuboid))
			{
				remainder.m_cuboids.erase(remainderCuboid);
				if(remainder.m_cuboids.empty())
					return true;
			}
		}
	}
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const OffsetCuboid& offsetCuboid) const
{
	if(offsetCuboid.hasAnyNegativeCoordinates())
		return false;
	return contains(Cuboid::create(offsetCuboid));
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
SmallSet<PointType> CuboidSetBase<CuboidType, PointType, CuboidSetType>::toPointSet() const
{
	SmallSet<PointType> output;
	for(const CuboidType& cuboid : m_cuboids)
		output.maybeInsertAll(cuboid);
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isAdjacent(const CuboidType& cuboid) const
{
	for(const auto& c : m_cuboids)
		if(c.isTouching(cuboid))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidType CuboidSetBase<CuboidType, PointType, CuboidSetType>::boundry() const
{
	PointType highest;
	PointType lowest;
	for(const CuboidType& cuboid : m_cuboids)
	{
		highest = highest.empty() ? cuboid.m_high : highest.max(cuboid.m_high);
		lowest = lowest.empty() ? cuboid.m_low : lowest.max(cuboid.m_low);
	}
	return {highest, lowest};
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getLowest() const
{
	PointType output{PointType::DimensionType::create(0), PointType::DimensionType::create(0), PointType::DimensionType::max()};
	for(const CuboidType& cuboid : m_cuboids)
			if(cuboid.m_low.z() < output.z())
				output = cuboid.m_low;
	assert(output.x().exists());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersection(const CuboidType& cuboid) const
{
	CuboidSetType output;
	for(const CuboidType& otherCuboid : m_cuboids)
		if(otherCuboid.intersects(cuboid))
			output.add(otherCuboid.intersection(cuboid));
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersectionPoint(const CuboidType& cuboid) const
{
	CuboidSetType output;
	for(const CuboidType& otherCuboid : m_cuboids)
		if(otherCuboid.intersects(cuboid))
			return otherCuboid.intersection(cuboid).m_high;
	return PointType::null();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersects(const CuboidType& cuboid) const
{
	for(const CuboidType& c : m_cuboids)
		if(cuboid.intersects(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersects(const CuboidSetType& cuboids) const
{
	for(const CuboidType& c : m_cuboids)
		if(cuboids.intersects(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouching(const CuboidType& cuboid) const
{
	for(const CuboidType& c : m_cuboids)
		if(cuboid.isTouching(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouchingFace(const CuboidType& cuboid) const
{
	for(const CuboidType& c : m_cuboids)
		if(cuboid.isTouchingFace(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouchingFaceFromInside(const CuboidType& cuboid) const
{
	for(const CuboidType& c : m_cuboids)
		if(cuboid.isTouchingFaceFromInside(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouching(const CuboidSetType& cuboids) const
{
	for(const CuboidType& c : m_cuboids)
		if(cuboids.isTouching(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isIntersectingOrAdjacentTo(const CuboidSetType& cuboids) const
{
	for(const CuboidType& c : cuboids)
		if(isIntersectingOrAdjacentTo(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isIntersectingOrAdjacentTo(const CuboidType& cuboid) const
{
	CuboidType inflated = cuboid;
	inflated.inflate({1});
	for(const CuboidType& c : m_cuboids)
		if(inflated.intersects(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
std::string CuboidSetBase<CuboidType, PointType, CuboidSetType>::toString() const
{
	return m_cuboids.toString();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(const SmallSet<PointType>& points)
{
	CuboidSetType output;
	for(const PointType& point : points)
		output.add({point, point});
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(const CuboidSetType& set)
{
	return set;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(const CuboidType& cuboid)
{
	return CuboidSetType::create(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(const PointType& point)
{
	return CuboidSetType::create(CuboidType::create(point, point));
}