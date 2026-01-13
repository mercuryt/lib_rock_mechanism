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
			maybeAdd(CuboidType{high, low});
		else
			maybeAdd(CuboidType{PointType::create(high), PointType::create(low)});
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
	int i = 0;
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
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::destroy(const int& index)
{
	m_cuboids.eraseIndex(index);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeAdd(const PointType& point)
{
	maybeAdd({point, point});
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeAddAll(const CuboidSetType& other)
{
	if(empty())
		for(const CuboidType& cuboid : other)
			m_cuboids.insert(cuboid);
	else
		for(const CuboidType& cuboid : other)
			maybeAdd(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeRemove(const PointType& point)
{
	maybeRemove({point, point});
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeRemove(const CuboidType& cuboid)
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
				add(fragment);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeAdd(const CuboidType& cuboid)
{
	maybeRemove(cuboid);
	insertOrMerge(cuboid);
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
		maybeAdd(cuboid);
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
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflate(const Distance& distance)
{
	auto copy = m_cuboids;
	clear();
	for(CuboidType& cuboid : copy)
	{
		cuboid.inflate(distance);
		maybeAdd(cuboid);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::mergeInternal(const CuboidType& absorbed, const int& absorber)
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
	int totalVolume = 0;
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
PointType::DimensionType CuboidSetBase<CuboidType, PointType, CuboidSetType>::highestZ() const
{
	const CuboidType& highest = std::ranges::min(m_cuboids.m_data, std::greater{}, [&](const CuboidType& cuboid) { return cuboid.m_high.z(); });
	return highest.m_high.z();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::empty() const { return m_cuboids.empty(); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::exists() const { return !m_cuboids.empty(); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
int CuboidSetBase<CuboidType, PointType, CuboidSetType>::size() const
{
	return m_cuboids.size();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
int CuboidSetBase<CuboidType, PointType, CuboidSetType>::volume() const
{
	int output = 0;
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
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const CuboidType& cuboid) const
{
	int remainingVolume = cuboid.volume();
	for(const CuboidType& other : m_cuboids)
		if(other.intersects(cuboid))
			remainingVolume -= other.intersection(cuboid).volume();
	return remainingVolume == 0;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const CuboidSetType& other) const
{
	for(const CuboidType& cuboid : other)
		if(!contains(cuboid))
			return false;
	return true;
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
const CuboidType& CuboidSetBase<CuboidType, PointType, CuboidSetType>::getCuboidContaining(const PointType& point) const
{
	for(const CuboidType& cuboid : m_cuboids)
		if(cuboid.contains(point))
			return cuboid;
	assert(false);
	std::unreachable();
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
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isAdjacent(const PointType& point) const
{
	for(const auto& cuboid : m_cuboids)
		if(cuboid.isTouching(point))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidType CuboidSetBase<CuboidType, PointType, CuboidSetType>::boundry() const
{
	assert(exists());
	PointType highest;
	PointType lowest;
	for(const CuboidType& cuboid : m_cuboids)
	{
		highest = highest.empty() ? cuboid.m_high : highest.max(cuboid.m_high);
		lowest = lowest.empty() ? cuboid.m_low : lowest.min(cuboid.m_low);
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
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersection(const CuboidSetType& other) const
{
	CuboidSetType output;
	for(const CuboidType& cuboid : m_cuboids)
		for(const CuboidType& otherCuboid : other)
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
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersects(const PointType& point) const
{
	for(const CuboidType& c : m_cuboids)
		if(c.intersects(point))
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
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getAdjacent() const
{
	assert(!empty());
	CuboidSetType derived;
	derived.m_cuboids = m_cuboids;
	CuboidSetType inflated = derived;
	inflated.inflate({1});
	inflated.removeAll(derived);
	return inflated;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getDirectlyAdjacent(const Distance& distance) const
{
	assert(!empty());
	CuboidSetType output;
	for(const CuboidType& cuboid : m_cuboids)
		for(Facing6 facing = Facing6::Below; facing != Facing6::Null; facing = (Facing6)((int)facing + 1))
		{
			CuboidType face = cuboid.getFace(facing);
			face.maybeShift(facing, distance);
			output.maybeAdd(face);
		}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflateFaces(const Distance& distance) const
{
	assert(!empty());
	CuboidSetType output;
	output.m_cuboids = this->m_cuboids;
	for(const CuboidType& cuboid : m_cuboids)
		for(Facing6 facing = Facing6::Below; facing != Facing6::Null; facing = (Facing6)((int)facing + 1))
		{
			CuboidType face = cuboid.getFace(facing);
				face.maybeShift(facing, distance);
			output.maybeAdd(face);
		}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::adjacentSlicedAtZ(const PointType::DimensionType& zLevel) const
{
	assert(boundry().m_high.z() >= zLevel);
	assert(boundry().m_low.z() <= zLevel);
	CuboidSetType output;
	// Iterate copies.
	for(CuboidType cuboid : m_cuboids)
	{
		if(cuboid.m_high.z() > zLevel || cuboid.m_low.z() < zLevel)
			continue;
		//TODO: Cuboid::inflateXAndY
		cuboid.inflate({1});
		// Make inflated cuboid into a flat slice.
		cuboid.m_high.setZ(zLevel);
		cuboid.m_low.setZ(zLevel);
		CuboidSetType notContained = CuboidSetType::create(cuboid);
		notContained.maybeRemoveAll(*this);
		output.maybeAddAll(notContained);
	}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::flattened(const PointType::DimensionType& zLevel) const
{
	CuboidSetType output;
	// Make a copy.
	for(CuboidType cuboid : m_cuboids)
	{
		cuboid.m_high.setZ(zLevel);
		cuboid.m_low.setZ(zLevel);
		output.maybeAdd(cuboid);
	}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::adjacentRecursive(const PointType& point) const
{
	// TODO: There are alot of redundant comparisons here. Could it be optimized by moving already found cuboids to the front of the vector and starting the search beyond them?
	CuboidSetType output;
	SmallSet<int> openList;
	SmallSet<int> candidates;
	int end = m_cuboids.size();
	for(int i = 0; i != end; ++i)
		if(m_cuboids[i].contains(point))
			openList.insert(i);
		else
			candidates.insert(i);
	assert(!openList.empty());
	while(!openList.empty())
	{
		const int currentIndex = openList.back();
		const CuboidType currentCuboid = m_cuboids[currentIndex];
		openList.popBack();
		int candidateEnd = candidates.size();
		SmallSet<int> toRemove;
		for(int i = 0; i != candidateEnd; ++i)
		{
			int candidateIndex = candidates[i];
			assert(candidateIndex != currentIndex);
			if(m_cuboids[candidateIndex].isTouching(currentCuboid))
			{
				toRemove.insert(i);
				openList.insert(candidateIndex);
				output.add(m_cuboids[candidateIndex]);
			}
			// Sort indices toRemove in descending order so as not to invalidate when moving from the back.
			std::sort(toRemove.m_data.begin(), toRemove.m_data.end(), std::greater<int>());
			for(int index : toRemove)
			{
				candidates[index] = candidates.back();
				candidates.popBack();
			}
		}
	}
	return output;
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
		output.add(CuboidType::create(point, point));
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