#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../../lib/Eigen/Dense"
#pragma GCC diagnostic pop
#include "../types.h"
#include "cuboid.h"

class CuboidSetSIMD
{
	using Coordinates = Eigen::ArrayX<DistanceInBlocksWidth>;
	Coordinates m_highX;
	Coordinates m_highY;
	Coordinates m_highZ;
	Coordinates m_lowX;
	Coordinates m_lowY;
	Coordinates m_lowZ;
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
};