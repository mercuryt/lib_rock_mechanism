#pragma once
#include "../geometry/cuboidSet.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/bitset.h"

class ClosedListWithFacing
{
	CuboidSet m_closedFromAllFacings;
	SmallMap<Point3D, BitSet<uint8_t, 4>> m_closedFromSomeFacings;
public:
	void close(const Point3D point, const Facing4 facing);
	void closeAllDirections(const Point3D point);
	void clear();
	// Returns true is point and facing were already closed.
	[[nodiscard]] bool checkAndCloseIfNot(const Point3D point, const Facing4 facing);
	void removeFrom(CuboidSet& cuboidSet) const;
	[[nodiscard]] bool check(const Point3D point, const Facing4 facing) const;
};