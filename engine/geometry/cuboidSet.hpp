#include "cuboidSet.h"
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetBase<CuboidType, PointType, CuboidSetType>::CuboidSetBase(PointType location, Facing4 rotation, const OffsetCuboidSet& offsetPairs)
{
	PointType coordinates = location;
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
	for(CuboidType cuboid : cuboids)
		m_cuboids.insert(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::insertOrMerge(CuboidType cuboid)
{
	int i = 0;
	for(CuboidType existing : m_cuboids)
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
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::destroy(int index)
{
	m_cuboids.eraseIndex(index);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeAdd(PointType point)
{
	maybeAdd({point, point});
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeAddAll(const CuboidSetType& other)
{
	if(empty())
		for(CuboidType cuboid : other)
			m_cuboids.insert(cuboid);
	else
		for(CuboidType cuboid : other)
			maybeAdd(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeRemove(PointType point)
{
	maybeRemove({point, point});
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeRemove(CuboidType cuboid)
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
			for(CuboidType fragment : existingCuboid.getChildrenWhenSplitBy(cuboid))
				add(fragment);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::maybeAdd(CuboidType cuboid)
{
	maybeRemove(cuboid);
	insertOrMerge(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::shift(Offset3D offset, Distance distance)
{
	for(CuboidType& cuboid : m_cuboids)
		cuboid.shift(offset, distance);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::shift(Facing6 facing, Distance distance)
{
	for(CuboidType& cuboid : m_cuboids)
		cuboid.shift(facing, distance);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::shifted(Offset3D offset, Distance distance) const
{
	CuboidSetType copy(static_cast<const CuboidSetType&>(*this));
	copy.shift(offset, distance);
	return copy;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::shiftedDirection(Facing6 facing, Distance distance) const
{
	CuboidSetType copy(static_cast<const CuboidSetType&>(*this));
	copy.shift(facing, distance);
	return copy;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::shiftWest(Distance distance)
{
	assert(boundry().m_low.x() != 0);
	Offset3D offset{-1, 0, 0};
	shift(offset, distance);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::shiftSouth(Distance distance)
{
	assert(boundry().m_low.y() != 0);
	Offset3D offset{0, -1, 0};
	shift(offset, distance);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::addSet(const CuboidSetType& other)
{
	for(CuboidType cuboid : other.getCuboids())
		maybeAdd(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::rotateAroundPoint(PointType point, Facing4 rotation)
{
	for(CuboidType& cuboid : m_cuboids)
		cuboid.rotateAroundPoint(point, rotation);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::swap(CuboidSetType& other) { other.m_cuboids.swap(m_cuboids); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::popBack() { m_cuboids.popBack(); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflate(Distance distance)
{
	auto copy = std::move(m_cuboids);
	clear();
	for(CuboidType cuboid : copy)
	{
		cuboid.inflate(distance);
		maybeAdd(cuboid);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflateVertical(Distance distance)
{
	auto copy = std::move(m_cuboids);
	clear();
	for(CuboidType cuboid : copy)
	{
		cuboid.inflateVertical(distance);
		maybeAdd(cuboid);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflateHorizontal(Distance distance)
{
	auto copy = std::move(m_cuboids);
	clear();
	for(CuboidType cuboid : copy)
	{
		cuboid.inflateHorizontal(distance);
		maybeAdd(cuboid);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflateDirection(Facing6 facing, Distance distance)
{
	auto copy = std::move(m_cuboids);
	clear();
	for(CuboidType cuboid : copy)
	{
		cuboid.inflateDirection(facing, distance);
		maybeAdd(cuboid);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::deflate(Distance distance)
{
	auto copy = std::move(m_cuboids);
	clear();
	for(CuboidType cuboid : copy)
	{
		cuboid.deflate(distance);
		add(cuboid);
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::mergeInternal(CuboidType absorbed, int absorber)
{
	CuboidType absorberCopy = m_cuboids[absorber];
	assert(absorbed.canMerge(absorberCopy));
	assert(absorberCopy.canMerge(absorbed));
	for(CuboidType existing : m_cuboids)
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
	for(CuboidType cuboid : m_cuboids)
	{
		totalVolume += cuboid.volume();
		for(PointType point : cuboid)
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
	CuboidType lowest = std::ranges::min(m_cuboids.m_data, {}, [&](CuboidType cuboid) { return cuboid.m_low.z(); });
	return lowest.m_low.z();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType::DimensionType CuboidSetBase<CuboidType, PointType, CuboidSetType>::highestZ() const
{
	CuboidType highest = std::ranges::min(m_cuboids.m_data, std::greater{}, [&](CuboidType cuboid) { return cuboid.m_high.z(); });
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
int64_t CuboidSetBase<CuboidType, PointType, CuboidSetType>::volume() const
{
	int output = 0;
	for(CuboidType cuboid : m_cuboids)
		output += cuboid.volume();
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(Point3D point) const
{
	for(CuboidType cuboid : m_cuboids)
		if(cuboid.contains(point))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(Offset3D point) const
{
	for(CuboidType cuboid : m_cuboids)
		if(cuboid.contains(point))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(CuboidType cuboid) const
{
	int remainingVolume = cuboid.volume();
	for(CuboidType other : m_cuboids)
		if(other.intersects(cuboid))
			remainingVolume -= other.intersection(cuboid).volume();
	return remainingVolume == 0;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::contains(const CuboidSetType& other) const
{
	for(CuboidType cuboid : other)
		if(!contains(cuboid))
			return false;
	return true;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
SmallSet<PointType> CuboidSetBase<CuboidType, PointType, CuboidSetType>::toPointSet() const
{
	SmallSet<PointType> output;
	for(CuboidType cuboid : m_cuboids)
		output.maybeInsertAll(cuboid);
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getCuboidContaining(PointType point) const
{
	for(CuboidType cuboid : m_cuboids)
		if(cuboid.contains(point))
			return cuboid;
	assert(false);
	std::unreachable();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isAdjacent(CuboidType cuboid) const
{
	for(const auto& c : m_cuboids)
		if(c.isTouching(cuboid))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isAdjacent(PointType point) const
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
	for(CuboidType cuboid : m_cuboids)
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
	for(CuboidType cuboid : m_cuboids)
			if(cuboid.m_low.z() < output.z())
				output = cuboid.m_low;
	assert(output.x().exists());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersection(CuboidType cuboid) const
{
	CuboidSetType output;
	for(CuboidType otherCuboid : m_cuboids)
		if(otherCuboid.intersects(cuboid))
			output.add(otherCuboid.intersection(cuboid));
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersection(const CuboidSetType& other) const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		for(CuboidType otherCuboid : other)
			if(otherCuboid.intersects(cuboid))
				output.add(otherCuboid.intersection(cuboid));
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersectionPoint(CuboidType cuboid) const
{
	CuboidSetType output;
	for(CuboidType otherCuboid : m_cuboids)
		if(otherCuboid.intersects(cuboid))
			return otherCuboid.intersection(cuboid).m_high;
	return PointType::null();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersects(CuboidType cuboid) const
{
	for(CuboidType c : m_cuboids)
		if(cuboid.intersects(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersects(PointType point) const
{
	for(CuboidType c : m_cuboids)
		if(c.intersects(point))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::intersects(const CuboidSetType& cuboids) const
{
	for(CuboidType c : m_cuboids)
		if(cuboids.intersects(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouching(CuboidType cuboid) const
{
	for(CuboidType c : m_cuboids)
		if(cuboid.isTouching(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouchingFace(CuboidType cuboid) const
{
	for(CuboidType c : m_cuboids)
		if(cuboid.isTouchingFace(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouchingFaceFromInside(CuboidType cuboid) const
{
	for(CuboidType c : m_cuboids)
		if(cuboid.isTouchingFaceFromInside(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isTouching(const CuboidSetType& cuboids) const
{
	for(CuboidType c : m_cuboids)
		if(cuboids.isTouching(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isIntersectingOrAdjacentTo(const CuboidSetType& cuboids) const
{
	for(CuboidType c : cuboids)
		if(isIntersectingOrAdjacentTo(c))
			return true;
	return false;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isIntersectingOrAdjacentTo(CuboidType cuboid) const
{
	CuboidType inflated = cuboid;
	inflated.inflate({1});
	for(CuboidType c : m_cuboids)
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
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getDirectlyAdjacent(Distance distance) const
{
	assert(!empty());
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		for(Facing6 facing = Facing6::Below; facing != Facing6::Null; facing = (Facing6)((int)facing + 1))
		{
			CuboidType face = cuboid.getFace(facing);
			face.maybeShift(facing, distance);
			output.maybeAdd(face);
		}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflateFaces(Distance distance) const
{
	assert(!empty());
	CuboidSetType output;
	output.m_cuboids = this->m_cuboids;
	for(CuboidType cuboid : m_cuboids)
		for(Facing6 facing = Facing6::Below; facing != Facing6::Null; facing = (Facing6)((int)facing + 1))
		{
			CuboidType face = cuboid.getFace(facing);
				face.maybeShift(facing, distance);
			output.maybeAdd(face);
		}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::inflated(Distance distance) const
{
	CuboidSetType output = *static_cast<const CuboidSetType*>(this);
	output.inflate(distance);
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::deflated(Distance distance) const
{
	CuboidSetType output = *static_cast<const CuboidSetType*>(this);
	output.deflate(distance);
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::slicedAtZ(PointType::DimensionType zLevel) const
{
	CuboidSetType output;
	// Iterate copies.
	for(CuboidType cuboid : m_cuboids)
	{
		if(cuboid.m_high.z() < zLevel || cuboid.m_low.z() > zLevel)
			continue;
		cuboid.m_high.setZ(zLevel);
		cuboid.m_low.setZ(zLevel);
		output.maybeAdd(cuboid);
	}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::sliceAtZ(PointType::DimensionType zLevel)
{
	m_cuboids = slicedAtZ(zLevel).m_cuboids;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
void CuboidSetBase<CuboidType, PointType, CuboidSetType>::prepare()
{
	CuboidType currentBoundry = boundry();
	if(volume() == currentBoundry.volume())
	{
		m_cuboids.resize(1);
		m_cuboids[0] = currentBoundry;
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::adjacentSlicedAtZ(PointType::DimensionType zLevel) const
{
	assert(boundry().m_high.z() >= zLevel);
	assert(boundry().m_low.z() <= zLevel);
	CuboidSetType output;
	// Iterate copies.
	for(CuboidType cuboid : m_cuboids)
	{
		if(cuboid.m_high.z() < zLevel || cuboid.m_low.z() > zLevel)
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
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::flattened(PointType::DimensionType zLevel) const
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
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFace(Facing6 facing) const
{
	switch(facing)
	{
		case(Facing6::East):
			return getFaceEast();
		case(Facing6::West):
			return getFaceWest();
		case(Facing6::South):
			return getFaceSouth();
		case(Facing6::North):
			return getFaceNorth();
		case(Facing6::Above):
			return getFaceAbove();
		case(Facing6::Below):
			return getFaceBelow();
		default:
			assert(false);
			std::unreachable();
	}
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFaceNorth() const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		output.add(cuboid.getFaceNorth());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFaceSouth() const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		output.add(cuboid.getFaceSouth());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFaceEast() const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		output.add(cuboid.getFaceEast());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFaceWest() const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		output.add(cuboid.getFaceWest());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFaceAbove() const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		output.add(cuboid.getFaceAbove());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getFaceBelow() const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		output.add(cuboid.getFaceBelow());
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::inverted() const
{
	CuboidSetType output = CuboidSetType::create(boundry());
	output.maybeRemoveAll(*this);
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
PointType CuboidSetBase<CuboidType, PointType, CuboidSetType>::nearestPointTo(CuboidType other) const
{
	assert(!empty());
	typename PointType::DimensionType nearest = PointType::DimensionType::max();
	PointType output;
	for(CuboidType cuboid : m_cuboids)
	{
		typename PointType::DimensionType distance = cuboid.distanceTo(other);
		if(distance < nearest)
		{
			distance = nearest;
			output = cuboid.nearestPointTo(other);
		}
	}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
template<typename ShapeT>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::adjacentRecursiveBody(ShapeT shape) const
{
	// TODO: There are alot of redundant comparisons here. Could it be optimized by moving already found cuboids to the front of the vector and starting the search beyond them?
	CuboidSetType output;
	SmallSet<int> openList;
	SmallSet<int> closedList;
	auto end = m_cuboids.size();
	for(int i = 0; i != end; ++i)
		if(m_cuboids[i].intersects(shape))
		{
			openList.insert(i);
			output.add(m_cuboids[i]);
			closedList.insert(i);
		}
	while(!openList.empty())
	{
		int currentIndex = openList.back();
		openList.popBack();
		CuboidType current = m_cuboids[currentIndex];
		for(int i = 0; i != end; ++i)
			if(m_cuboids[i].isTouching(current) && !closedList.contains(i))
			{
				openList.insert(i);
				output.add(m_cuboids[i]);
				closedList.insert(i);
			}
	}
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::adjacentRecursive(CuboidType shape) const { return adjacentRecursiveBody(shape); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::adjacentRecursive(PointType shape) const { return adjacentRecursiveBody(shape); }
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isContiguous() const
{
	assert(!m_cuboids.empty());
	if(m_cuboids.size() == 1)
		return true;
	for(int i{0}; i != m_cuboids.size() - 1; ++i)
		for(int j{i + 1}; j != m_cuboids.size(); ++j)
			if(!m_cuboids[i].isTouching(m_cuboids[j]))
				return false;
	return true;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::getContainedWhichTouchesFromInside(CuboidType shape) const
{
	CuboidSetType output;
	for(CuboidType cuboid : m_cuboids)
		if(cuboid.isTouchingFaceFromInside(shape))
			output.add(cuboid);
	shape.deflate();
	output.maybeRemove(shape);
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
bool CuboidSetBase<CuboidType, PointType, CuboidSetType>::isEqual(const CuboidSetType& other) const
{
	return
		other.volume() == volume() && contains(other);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
std::string CuboidSetBase<CuboidType, PointType, CuboidSetType>::toS() const
{
	return m_cuboids.toS();
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(const SmallSet<PointType>& points)
{
	CuboidSetType output;
	for(PointType point : points)
		output.add(CuboidType::create(point, point));
	return output;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(const CuboidSetType& set)
{
	return set;
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(CuboidType cuboid)
{
	return CuboidSetType::create(cuboid);
}
template<typename CuboidType, typename PointType, typename CuboidSetType>
CuboidSetType CuboidSetBase<CuboidType, PointType, CuboidSetType>::create(PointType point)
{
	return CuboidSetType::create(CuboidType::create(point, point));
}