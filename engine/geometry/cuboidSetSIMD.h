#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include "../types.h"
#include "cuboid.h"

class CuboidSetSIMD
{
	using Data = Eigen::Array<DistanceInBlocksWidth, 3, Eigen::Dynamic>;
	Data m_dataHigh;
	Data m_dataLow;
	Cuboid m_boundingBox;
	uint m_size = 0;
	uint m_capacity = 0;
public:
	CuboidSetSIMD(uint capacity) { assert(capacity != 0); reserve(capacity); }
	void reserve(uint capacity);
	void insert(const Cuboid& cuboid);
	void clear();
	[[nodiscard]] bool intersects(const Cuboid& cuboid) const;
	[[nodiscard]] uint size() const { return m_size; }
	[[nodiscard]] uint capacity() const { return m_capacity; }
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Cuboid& cuboid);
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfContainedCuboids(const Cuboid& cuboid);
	[[nodiscard]] Eigen::Array<bool, 1, Eigen::Dynamic> indicesOfIntersectingCuboids(const Sphere& sphere);
	[[nodiscard]] const Cuboid& getBoundingBox() const { return m_boundingBox; }
};