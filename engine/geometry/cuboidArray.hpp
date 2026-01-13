#pragma once
#include "cuboidArray.h"
#include "paramaterizedLine.h"
#include "sphere.h"
#include "cuboidSet.h"


template<int capacity>
CuboidArray<capacity>::CuboidArray() : m_high(Distance::null().get()), m_low(Distance::null().get()) { }
template<int capacity>
void CuboidArray<capacity>::insert(const int& index, const Cuboid& cuboid)
{
	m_high.col(index) = cuboid.m_high.data;
	m_low.col(index) = cuboid.m_low.data;
}
template<int capacity>
void CuboidArray<capacity>::erase(const int& index)
{
	m_high.col(index) = Distance::null().get();
	m_low.col(index) = Distance::null().get();
}
template<int capacity>
void CuboidArray<capacity>::clear()
{
	m_high = Distance::null().get();
	m_low = Distance::null().get();
}
template<int capacity>
bool CuboidArray<capacity>::operator==(const CuboidArray& other) const
{
	return (m_high == other.m_high).all() && (m_low == other.m_low).all();
}
template<int capacity>
bool CuboidArray<capacity>::contains(const Cuboid& cuboid) const
{
	for(const Cuboid& otherCuboid : *this)
		if(cuboid == otherCuboid)
			return true;
	return false;
}
template<int capacity>
int CuboidArray<capacity>::getCapacity() const { return capacity; }
template<int capacity>
const Cuboid CuboidArray<capacity>::operator[](const int& index) const { return {Coordinates(m_high.col(index)), Coordinates(m_low.col(index)) }; }
template<int capacity>
Cuboid CuboidArray<capacity>::boundry() const
{
	// Null cuboids need to be filtered from the max side of the boundry because we use a large number for null.
	PointArray copyMax = m_high;
	const BoolArray isNotNull = !(m_high == Distance::null().get()).colwise().any();
	Eigen::Array<DistanceWidth, 1, capacity> isNotNullInt = isNotNull.template cast<DistanceWidth>();
	copyMax *= isNotNullInt.replicate(3, 1);
	return {
		Point3D(copyMax.rowwise().maxCoeff()),
		Point3D(m_low.rowwise().minCoeff())
	};
}
template<int capacity>
bool CuboidArray<capacity>::intersects(const Cuboid& cuboid) const
{
	return indicesOfIntersectingCuboids(cuboid).any();
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboids(const Point3D& point) const
{
	return (
		(m_high >= point.data.replicate(1, capacity)).colwise().all() &&
		(m_low <= point.data.replicate(1, capacity)).colwise().all()
	);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboids(const Cuboid& cuboid) const
{
	return (
		(m_high >= cuboid.m_low.data.replicate(1, capacity)).colwise().all() &&
		(m_low <= cuboid.m_high.data.replicate(1, capacity)).colwise().all()
	);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboids(const CuboidSet& cuboids) const
{
	BoolArray output;
	output.fill(0);
	for(const Cuboid& cuboid : cuboids)
		output += indicesOfIntersectingCuboids(cuboid);
	return output;
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfContainedCuboids(const Cuboid& cuboid) const
{
	return (
		(m_high <= cuboid.m_high.data.replicate(1, capacity)).colwise().all() &&
		(m_low >= cuboid.m_low.data.replicate(1, capacity)).colwise().all()
	);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfContainedCuboids(const CuboidSet& cuboids) const
{
	BoolArray output;
	output.fill(0);
	for(const Cuboid& cuboid : cuboids)
		output += indicesOfContainedCuboids(cuboid);
	return output;
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfCuboidsContaining(const Cuboid& cuboid) const
{
	return (
		(m_high >= cuboid.m_high.data.replicate(1, capacity)).colwise().all() &&
		(m_low <= cuboid.m_low.data.replicate(1, capacity)).colwise().all()
	);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfCuboidsContaining(const CuboidSet& cuboids) const
{
	BoolArray output;
	output.fill(0);
	for(const Cuboid& cuboid : cuboids)
		output += indicesOfCuboidsContaining(cuboid);
	return output;
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboids(const Sphere& sphere) const
{
	int radius = sphere.radius.get();
	const Eigen::Array<int, 3, 1>& center = sphere.center.data.template cast<int>();
	return !(
		((m_high.template cast<int>() + radius) < center.replicate(1, capacity)).colwise().any() ||
		((m_low.template cast<int>() - radius) > center.replicate(1, capacity)).colwise().any()
	);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfContainedCuboids(const Sphere& sphere) const
{
	int radius = sphere.radius.get();
	const Eigen::Array<int, 3, 1>& center = sphere.center.data.cast<int>();
	return !(
		((m_high.template cast<int>() - radius) >= center.replicate(1, capacity)).colwise().any() &&
		((m_low.template cast<int>() + radius) <= center.replicate(1, capacity)).colwise().any()
	);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboids(const ParamaterizedLine& line) const
{
	const PointArray replicatedStart = line.begin.data.replicate(1, capacity);
	const Float3DArray replicatedSloap = line.sloap.replicate(1, capacity);
	const PointArray replicatedHighBoundry = line.boundry.m_high.data.replicate(1, capacity);
	const PointArray replicatedLowBoundry = line.boundry.m_low.data.replicate(1, capacity);
	const Offset3DArray distanceFromStartToHighFace = m_high.template cast<OffsetWidth>() - replicatedStart.template cast<OffsetWidth>();
	const Float3DArray stepsFromStartToHighFace = distanceFromStartToHighFace.template cast<float>() / replicatedSloap;
	const Offset3DArray distanceFromStartToLowFace = m_low.template cast<OffsetWidth>() - replicatedStart.template cast<OffsetWidth>();
	const Float3DArray stepsFromStartToLowFace = distanceFromStartToLowFace.template cast<float>() / replicatedSloap;
	BoolArray result;
	result.setZero();
	// X
	if(line.sloap.x() != 0.0f)
	{
		const PointArray coordinatesAtHighX = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(0).replicate(3, 1)).round().template cast<DistanceWidth>();
		result +=
			// Check Y.
			coordinatesAtHighX.row(1) <= m_high.row(1) &&
			coordinatesAtHighX.row(1) >= m_low.row(1) &&
			// Check Z.
			coordinatesAtHighX.row(2) <= m_high.row(2) &&
			coordinatesAtHighX.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtHighX <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtHighX >= replicatedLowBoundry).colwise().all();
		const PointArray coordinatesAtLowX = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(0).replicate(3, 1)).round().template cast<DistanceWidth>();
		result +=
			// Check Y.
			coordinatesAtLowX.row(1) <= m_high.row(1) &&
			coordinatesAtLowX.row(1) >= m_low.row(1) &&
			// Check Z.
			coordinatesAtLowX.row(2) <= m_high.row(2) &&
			coordinatesAtLowX.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtLowX <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtLowX >= replicatedLowBoundry).colwise().all();
	}
	// Y
	if(line.sloap.y() != 0.0f)
	{
		const PointArray coordinatesAtHighY = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(1).replicate(3, 1)).round().template cast<DistanceWidth>();
		result +=
			// Check X.
			coordinatesAtHighY.row(0) <= m_high.row(0) &&
			coordinatesAtHighY.row(0) >= m_low.row(0) &&
			// Check Z.
			coordinatesAtHighY.row(2) <= m_high.row(2) &&
			coordinatesAtHighY.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtHighY <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtHighY >= replicatedLowBoundry).colwise().all();
		const PointArray coordinatesAtLowY = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(1).replicate(3, 1)).round().template cast<DistanceWidth>();
		result +=
			// Check X.
			coordinatesAtLowY.row(0) <= m_high.row(0) &&
			coordinatesAtLowY.row(0) >= m_low.row(0) &&
			// Check Z.
			coordinatesAtLowY.row(2) <= m_high.row(2) &&
			coordinatesAtLowY.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtLowY <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtLowY >= replicatedLowBoundry).colwise().all();
	}
	// Z
	if(line.sloap.z() != 0.0f)
	{
		const PointArray coordinatesAtHighZ = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(2).replicate(3, 1)).round().template cast<DistanceWidth>();
		result +=
			// Check X.
			coordinatesAtHighZ.row(0) <= m_high.row(0) &&
			coordinatesAtHighZ.row(0) >= m_low.row(0) &&
			// Check Y.
			coordinatesAtHighZ.row(1) <= m_high.row(1) &&
			coordinatesAtHighZ.row(1) >= m_low.row(1) &&
			// Check range.
			(coordinatesAtHighZ <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtHighZ >= replicatedLowBoundry).colwise().all();
		const PointArray coordinatesAtLowZ = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(2).replicate(3, 1)).round().template cast<DistanceWidth>();
		result +=
			// Check X.
			coordinatesAtLowZ.row(0) <= m_high.row(0) &&
			coordinatesAtLowZ.row(0) >= m_low.row(0) &&
			// Check Y.
			coordinatesAtLowZ.row(1) <= m_high.row(1) &&
			coordinatesAtLowZ.row(1) >= m_low.row(1) &&
			// Check range.
			(coordinatesAtLowZ <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtLowZ >= replicatedLowBoundry).colwise().all();
	}
	return result;
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboidsWhereThereIsADifferenceInEntranceAndExitZ(const ParamaterizedLine& line) const
{
	assert(line.sloap.z() != 0);
	const PointArray replicatedStart = line.begin.data.replicate(1, capacity);
	const Float3DArray replicatedSloap = line.sloap.replicate(1, capacity);
	const PointArray replicatedHighBoundry = line.boundry.m_high.data.replicate(1, capacity);
	const PointArray replicatedLowBoundry = line.boundry.m_low.data.replicate(1, capacity);
	// Check for intersection with high faces.
	const Offset3DArray distanceFromStartToHighFace = m_high.template cast<OffsetWidth>() - replicatedStart.template cast<OffsetWidth>();
	const Float3DArray stepsFromStartToHighFace = distanceFromStartToHighFace.template cast<float>() / replicatedSloap;
	// High x.
	Bool3DArray highResults;
	if(line.sloap[0] == 0.0f)
		highResults.row(0) = false;
	else
	{
		const PointArray coordinatesAtHighX = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(0).replicate(3, 1)).round().template cast<DistanceWidth>();
		highResults.row(0) =
			// Check Y.
			coordinatesAtHighX.row(1) <= m_high.row(1) &&
			coordinatesAtHighX.row(1) >= m_low.row(1) &&
			// Check Z.
			coordinatesAtHighX.row(2) <= m_high.row(2) &&
			coordinatesAtHighX.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtHighX <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtHighX >= replicatedLowBoundry).colwise().all();
	}
	// High y.
	if(line.sloap[1] == 0.0f)
		highResults.row(1) = false;
	else
	{
		const PointArray coordinatesAtHighY = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(1).replicate(3, 1)).round().template cast<DistanceWidth>();
		highResults.row(1) =
			// Check X.
			coordinatesAtHighY.row(0) <= m_high.row(0) &&
			coordinatesAtHighY.row(0) >= m_low.row(0) &&
			// Check Z.
			coordinatesAtHighY.row(2) <= m_high.row(2) &&
			coordinatesAtHighY.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtHighY <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtHighY >= replicatedLowBoundry).colwise().all();
	}
	if(line.sloap[2] == 0.0f)
	// High z.
		highResults.row(2) = false;
	else
	{
		const PointArray coordinatesAtHighZ = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(2).replicate(3, 1)).round().template cast<DistanceWidth>();
		highResults.row(2) =
			// Check X.
			coordinatesAtHighZ.row(0) <= m_high.row(0) &&
			coordinatesAtHighZ.row(0) >= m_low.row(0) &&
			// Check Y.
			coordinatesAtHighZ.row(1) <= m_high.row(1) &&
			coordinatesAtHighZ.row(1) >= m_low.row(1) &&
			// Check range.
			(coordinatesAtHighZ <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtHighZ >= replicatedLowBoundry).colwise().all();
	}
	// Check for intersection with low faces.
	const Offset3DArray distanceFromStartToLowFace = m_low.template cast<OffsetWidth>() - replicatedStart.template cast<OffsetWidth>();
	const Float3DArray stepsFromStartToLowFace = distanceFromStartToLowFace.template cast<float>() / replicatedSloap;
	Bool3DArray lowResults;
	// Low x.
	if(line.sloap[0] == 0.0f)
		lowResults.row(0) = false;
	else
	{
		const PointArray coordinatesAtLowX = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(0).replicate(3, 1)).round().template cast<DistanceWidth>();
		lowResults.row(0) =
			// Check Y.
			coordinatesAtLowX.row(1) <= m_high.row(1) &&
			coordinatesAtLowX.row(1) >= m_low.row(1) &&
			// Check Z.
			coordinatesAtLowX.row(2) <= m_high.row(2) &&
			coordinatesAtLowX.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtLowX <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtLowX >= replicatedLowBoundry).colwise().all();
	}
	// Low y.
	if(line.sloap[1] == 0.0f)
		lowResults.row(1) = false;
	else
	{
		const PointArray coordinatesAtLowY = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(1).replicate(3, 1)).round().template cast<DistanceWidth>();
		lowResults.row(1) =
			// Check X.
			coordinatesAtLowY.row(0) <= m_high.row(0) &&
			coordinatesAtLowY.row(0) >= m_low.row(0) &&
			// Check Z.
			coordinatesAtLowY.row(2) <= m_high.row(2) &&
			coordinatesAtLowY.row(2) >= m_low.row(2) &&
			// Check range.
			(coordinatesAtLowY <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtLowY >= replicatedLowBoundry).colwise().all();
	}
	// Low z.
	// TODO: Unnessesary?
	if(line.sloap[2] == 0.0f)
		lowResults.row(2) = false;
	else
	{
		const PointArray coordinatesAtLowZ = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(2).replicate(3, 1)).round().template cast<DistanceWidth>();
		lowResults.row(2) =
			// Check X.
			coordinatesAtLowZ.row(0) <= m_high.row(0) &&
			coordinatesAtLowZ.row(0) >= m_low.row(0) &&
			// Check Y.
			coordinatesAtLowZ.row(1) <= m_high.row(1) &&
			coordinatesAtLowZ.row(1) >= m_low.row(1) &&
			// Check range.
			(coordinatesAtLowZ <= replicatedHighBoundry).colwise().all() &&
			(coordinatesAtLowZ >= replicatedLowBoundry).colwise().all();
	}
	return highResults.colwise().any() || lowResults.colwise().any();
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboidsLowZOnly(const Point3D& point) const { return indicesOfIntersectingCuboids(point); }
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboidsLowZOnly(const Cuboid& cuboid) const { return indicesOfIntersectingCuboids(cuboid); }
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboidsLowZOnly(const Sphere& sphere) const { return indicesOfIntersectingCuboids(sphere); }
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboidsLowZOnly(const ParamaterizedLine& line) const
{
	// Cannot intersect z face with no z slope.
	if(line.sloap[2] == 0.0f)
	{
		BoolArray output;
		output.fill(0);
		return output;
	}
	const PointArray replicatedStart = line.begin.data.replicate(1, capacity);
	const Float3DArray replicatedSloap = line.sloap.replicate(1, capacity);
	const PointArray replicatedHighBoundry = line.boundry.m_high.data.replicate(1, capacity);
	const PointArray replicatedLowBoundry = line.boundry.m_low.data.replicate(1, capacity);
	const OffsetArray distanceFromStartToLowZFace = m_low.row(2).template cast<OffsetWidth>() - replicatedStart.row(2).template cast<OffsetWidth>();
	const FloatArray stepsFromStartToLowZFace = distanceFromStartToLowZFace.template cast<float>() / replicatedSloap.row(2);
	const PointArray coordinatesAtLowZ = replicatedStart + (replicatedSloap * stepsFromStartToLowZFace.replicate(3, 1)).round().template cast<DistanceWidth>();
	BoolArray results =
		// Check X.
		coordinatesAtLowZ.row(0) <= m_high.row(0) &&
		coordinatesAtLowZ.row(0) >= m_low.row(0) &&
		// Check Y.
		coordinatesAtLowZ.row(1) <= m_high.row(1) &&
		coordinatesAtLowZ.row(1) >= m_low.row(1) &&
		// Check range.
		(coordinatesAtLowZ <= replicatedHighBoundry).colwise().all() &&
		(coordinatesAtLowZ >= replicatedLowBoundry).colwise().all();
	return results.colwise().any();
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfIntersectingCuboidsLowZOnly(const CuboidSet& cuboids) const
{
	BoolArray output;
	output.fill(0);
	for(const Cuboid& cuboid : cuboids)
		output += indicesOfIntersectingCuboidsLowZOnly(cuboid);
	return output;
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfMergeableCuboids(const Cuboid& cuboid) const
{
	const Bool3DArray sharedAxesHigh = m_high == cuboid.m_high.data.replicate(1, capacity);
	const Bool3DArray sharedAxesLow = m_low == cuboid.m_low.data.replicate(1, capacity);
	const Bool3DArray highAndLowAreShared = sharedAxesHigh && sharedAxesLow;
	const Eigen::Array<int, 3, capacity> highAndLowAreSharedInteger = highAndLowAreShared.template cast<int>();
	const Eigen::Array<int, 1, capacity> countOfSharedAxes = highAndLowAreSharedInteger.colwise().sum();
	const BoolArray twoAxesAreShared = countOfSharedAxes == 2;
	return twoAxesAreShared && indicesOfTouchingCuboids(cuboid);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfTouchingCuboids(const Cuboid& cuboid) const
{
	// Use plus one on each side of the comparitor operator to avoid subtraction as these are unsigned coordinates.
	const BoolArray high = ((cuboid.m_high.data.replicate(1, capacity) + 1 < m_low)).colwise().any();
	const BoolArray low = (cuboid.m_low.data.replicate(1, capacity) > (m_high + 1)).colwise().any();
	return !(high || low);
}
template<int capacity>
CuboidArray<capacity>::BoolArray CuboidArray<capacity>::indicesOfTouchingCuboids(const CuboidSet& cuboids) const
{
	BoolArray output;
	output.fill(0);
	for(const Cuboid& cuboid : cuboids)
		output += indicesOfTouchingCuboids(cuboid);
	return output;
}
template<int capacity>
int CuboidArray<capacity>::indexOfCuboid(const Cuboid& cuboid) const
{
	const PointArray replicatedHigh = cuboid.m_high.data.replicate(1, capacity);
	const BoolArray high = (m_high == replicatedHigh).colwise().all();
	const PointArray replicatedLow = cuboid.m_low.data.replicate(1, capacity);
	const BoolArray low = (m_low == replicatedLow).colwise().all();
	const BoolArray match = high && low;
	if(!match.any())
		return capacity;
	for(int i = 0; i != capacity; ++i)
		if(match.coeff(i))
			return i;
	std::unreachable();
}
template<int capacity>
bool CuboidArray<capacity>::anyOverlap() const
{
	const auto& beginCuboids = begin();
	const auto& endCuboids = end();
	const auto oneBeforeEnd = endCuboids - 1;
	for(auto outer = beginCuboids; outer != oneBeforeEnd; ++outer)
		for(auto inner = outer + 1; inner != endCuboids; ++inner)
			if((*outer) != (*inner) && (*outer).intersects(*inner))
				return true;
	return false;
}
template<int capacity>
std::string CuboidArray<capacity>::toString() const
{
	SmallSet<Cuboid> cuboids;
	std::string output;
	for(const Cuboid& cuboid : *this)
		if(cuboid.exists())
			cuboids.insert(cuboid);
	cuboids.sort();
	for(const Cuboid& cuboid : cuboids)
		output += cuboid.toString() + " ";
	return output;
}
template<int capacity>
Cuboid CuboidArray<capacity>::at(int i) const { return (*this)[i]; }