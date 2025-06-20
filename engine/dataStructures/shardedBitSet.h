#pragma once
#include "../numericTypes/types.h"
#include "../geometry/point3D.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop

//Shard width should be a power of 2.
template<uint shardWidth = 16>
class ShardedBitSet
{
	Eigen::Array<DistanceInBlocksWidth, 3> m_convertToIndex;
	std::vector<bool> m_data;
	ShardedBitSet(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z)
	{
		m_data.resize((x * y * z).get());
		const uint shard = x / shardWidth;
		m_convertToIndex = {(x * y).get() , x.get(), 1};
	}
	uint convertPoint(const Point3D& point)
	{
		const uint shard = point.x() / shardWidth;
		auto copy = point.data;
		assert(copy[0] >= shard * shardWidth);
		copy[0] -= shard * shardWidth;
		copy[1] +=  m_ySize * shard;
		return (copy * m_convertToIndex).sum();
	}
public:
	void maybeSet(const Point3D& point) { m_data[convertPoint(point)] = true; }
	void maybeUnset(const Point3D& point) { m_data[convertPoint(point)] = false; }
	void set(const Point3D& point) { assert(!check(point)); maybeSet(point); }
	void unset(const Point3D& point) { assert(check(point)); maybeUnset(point); }
	[[nodiscard]] bool check(const Point3D& point) { return m_data[convertPoint(point)]; }
};