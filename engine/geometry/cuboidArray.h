#pragma once
#include "cuboid.h"
#include "cuboidSetSIMD.h"
#include "paramaterizedLine.h"

template<int capacity>
class CuboidArray
{
	using BoolArray = Eigen::Array<bool, 1, capacity>;
	using Bool3DArray = Eigen::Array<bool, 3, capacity>;
	using FloatArray = Eigen::Array<float, 1, capacity>;
	using Float3DArray = Eigen::Array<float, 3, capacity>;
	using PointArray = Eigen::Array<DistanceInBlocksWidth, 3, capacity>;
	using OffsetArray = Eigen::Array<OffsetWidth, 3, capacity>;
	PointArray m_high;
	PointArray m_low;
public:
	CuboidArray() : m_high(DistanceInBlocks::null().get()), m_low(DistanceInBlocks::null().get()) { }
	CuboidArray(const CuboidArray&) = default;
	CuboidArray(CuboidArray&&) = default;
	CuboidArray& operator=(const CuboidArray&) = default;
	CuboidArray& operator=(CuboidArray&&) = default;
	void insert(const uint16_t& index, const Cuboid& cuboid)
	{
		m_high.col(index) = cuboid.m_highest.data;
		m_low.col(index) = cuboid.m_lowest.data;
	}
	void erase(const uint16_t& index)
	{
		uint last = size() - 1;
		if(index != last)
			insert(index, (*this)[last]);
		insert(index, {});
		assert(last == (uint)size());
	}
	void eraseByMask(const BoolArray& mask)
	{
		auto copy = *this;
		uint end = size();
		clear();
		for(uint i = 0; i < end; ++i)
			if(!mask[i])
				pushBack(copy[i]);
	}
	void clear()
	{
		m_high = DistanceInBlocks::null().get();
		m_low = DistanceInBlocks::null().get();
	}
	void pushBack(const Cuboid& cuboid)
	{
		uint index = size();
		insert(index, cuboid);
	}
	[[nodiscard]] const Cuboid operator[](const uint& index) const { return {Coordinates(m_high.col(index)), Coordinates(m_low.col(index)) }; }
	[[nodiscard]] int size() const
	{
		return (m_high.row(0) != DistanceInBlocks::null().get()).template cast<uint8_t>().sum();
	}
	[[nodiscard]] Cuboid boundry() const { return {Point3D(m_high.rowwise().maxCoeff()), Point3D(m_low.rowwise().minCoeff())}; }
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Point3D& point) const
	{
		return (
			(m_high >= point.data.replicate(1, capacity)).colwise().all() &&
			(m_low <= point.data.replicate(1, capacity)).colwise().all()
		);
	}
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Cuboid& cuboid) const
	{
		return (
			(m_high >= cuboid.m_lowest.data.replicate(1, capacity)).colwise().all() &&
			(m_low <= cuboid.m_highest.data.replicate(1, capacity)).colwise().all()
		);
	}
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const Cuboid& cuboid) const
	{
		return (
			(m_high <= cuboid.m_highest.data.replicate(1, capacity)).colwise().all() &&
			(m_low >= cuboid.m_lowest.data.replicate(1, capacity)).colwise().all()
		);
	}
	[[nodiscard]] BoolArray indicesOfCuboidsContaining(const Cuboid& cuboid) const
	{
		return (
			(m_high >= cuboid.m_highest.data.replicate(1, capacity)).colwise().all() &&
			(m_low <= cuboid.m_lowest.data.replicate(1, capacity)).colwise().all()
		);
	}
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const Sphere& sphere) const
	{
		int radius = sphere.radius.get();
		const Eigen::Array<int, 3, 1>& center = sphere.center.data.template cast<int>();
		return !(
			((m_high.template cast<int>() + radius) < center.replicate(1, capacity)).colwise().any() ||
			((m_low.template cast<int>() - radius) > center.replicate(1, capacity)).colwise().any()
		);
	}
	[[nodiscard]] BoolArray indicesOfContainedCuboids(const Sphere& sphere) const
	{
		int radius = sphere.radius.get();
		const Eigen::Array<int, 3, 1>& center = sphere.center.data.cast<int>();
		return !(
			((m_high.template cast<int>() - radius) >= center.replicate(1, capacity)).colwise().any() &&
			((m_low.template cast<int>() + radius) <= center.replicate(1, capacity)).colwise().any()
		);
	}
	[[nodiscard]] BoolArray indicesOfIntersectingCuboids(const ParamaterizedLine& line) const
	{
		const PointArray replicatedStart = line.begin.data.replicate(1, capacity);
		const Float3DArray replicatedSloap = line.sloap.replicate(1, capacity);
		const PointArray replicatedHighBoundry = line.boundry.m_highest.data.replicate(1, capacity);
		const PointArray replicatedLowBoundry = line.boundry.m_lowest.data.replicate(1, capacity);
		// Check for intersection with high faces.
		const OffsetArray distanceFromStartToHighFace = m_high.template cast<OffsetWidth>() - replicatedStart.template cast<OffsetWidth>();
		const Float3DArray stepsFromStartToHighFace = distanceFromStartToHighFace.template cast<float>() / replicatedSloap;
		// High x.
		Bool3DArray highResults;
		if(line.sloap[0] == 0.0f)
			highResults.row(0) = false;
		else
		{
			const PointArray coordinatesAtHighX = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(0).replicate(3, 1)).template cast<DistanceInBlocksWidth>();
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
			const PointArray coordinatesAtHighY = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(1).replicate(3, 1)).template cast<DistanceInBlocksWidth>();
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
			const PointArray coordinatesAtHighZ = replicatedStart + (replicatedSloap * stepsFromStartToHighFace.row(2).replicate(3, 1)).template cast<DistanceInBlocksWidth>();
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
		const OffsetArray distanceFromStartToLowFace = m_low.template cast<OffsetWidth>() - replicatedStart.template cast<OffsetWidth>();
		const Float3DArray stepsFromStartToLowFace = distanceFromStartToLowFace.abs().template cast<float>() / replicatedSloap;
		Bool3DArray lowResults;
		// Low x.
		if(line.sloap[0] == 0.0f)
			highResults.row(0) = false;
		else
		{
			const PointArray coordinatesAtLowX = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(0).replicate(3, 1)).template cast<DistanceInBlocksWidth>();
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
			highResults.row(1) = false;
		else
		{
			const PointArray coordinatesAtLowY = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(1).replicate(3, 1)).template cast<DistanceInBlocksWidth>();
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
			highResults.row(2) = false;
		else
		{
			const PointArray coordinatesAtLowZ = replicatedStart + (replicatedSloap * stepsFromStartToLowFace.row(2).replicate(3, 1)).template cast<DistanceInBlocksWidth>();
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
	[[nodiscard]] BoolArray indicesOfMergeableCuboids(const Cuboid& cuboid) const
	{
		const Bool3DArray sharedAxesHigh = m_high == cuboid.m_highest.data.replicate(1, capacity);
		const Bool3DArray sharedAxesLow = m_low == cuboid.m_lowest.data.replicate(1, capacity);
		const Bool3DArray highAndLowAreShared = sharedAxesHigh && sharedAxesLow;
		const Eigen::Array<uint8_t, 3, capacity> highAndLowAreSharedInteger = highAndLowAreShared.template cast<uint8_t>();
		const Eigen::Array<uint8_t, 1, capacity> countOfSharedAxes = highAndLowAreSharedInteger.colwise().sum();
		const BoolArray twoAxesAreShared = countOfSharedAxes == 2;
		return twoAxesAreShared && indicesOfTouchingCuboids(cuboid);
	}
	[[nodiscard]] BoolArray indicesOfTouchingCuboids(const Cuboid& cuboid) const
	{
		// Use plus one on each side of the comparitor operator to avoid subtraction as these are unsigned coordinates.
		const BoolArray high = ((cuboid.m_highest.data.replicate(1, capacity) + 1 < m_low)).colwise().any();
		const BoolArray low = (cuboid.m_lowest.data.replicate(1, capacity) > (m_high + 1)).colwise().any();
		return !(high || low);
	}
	class ConstIterator
	{
		const CuboidSetSIMD& m_set;
		uint m_index;
	public:
		ConstIterator(const CuboidSetSIMD& set, const uint& index) : m_set(set), m_index(index) { }
		void operator++() { ++m_index; }
		[[nodiscard]] ConstIterator operator++(int) { auto copy = *this; ++(*this); return copy; }
		[[nodiscard]] Cuboid operator*() const { return m_set[m_index]; }
		[[nodiscard]] bool operator==(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index == other.m_index; }
		[[nodiscard]] bool operator!=(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index != other.m_index; }
		[[nodiscard]] std::strong_ordering operator<=>(const ConstIterator& other) { assert(&m_set == &other.m_set); return m_index <=> other.m_index; }
	};
	ConstIterator begin() const { return ConstIterator(*this, 0); }
	ConstIterator end() const { return ConstIterator(*this, size()); }
};