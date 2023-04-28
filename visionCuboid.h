/*
 * A cuboid of space where every block has line of sight to every other block.
 * Used to skip line of sight calculations.
 * Stored in DerivedArea::m_visionCuboids. Deleted durring DerivedArea::writeStep if m_destroy is true.
 */
#pragma once
#include <cstdint>
#include "cuboid.h"
class VisionCuboid
{
public:
	static void setup(DerivedArea& area);
	static void clearDestroyed(DerivedArea& area);
	static void BlockIsNeverOpaque(DerivedBlock& block);
	static void BlockIsSometimesOpaque(DerivedBlock& block);
	static void BlockFloorIsNeverOpaque(DerivedBlock& block);
	static void BlockFloorIsSometimesOpaque(DerivedBlock& block);
	static VisionCuboid* getTargetToCombineWith(const Cuboid& cuboid);

	Cuboid m_cuboid;
	bool m_destroy;

	VisionCuboid(Cuboid& cuboid);
	bool canSeeInto(const Cuboid& cuboid) const;
	bool canCombineWith(const Cuboid& cuboid) const;
	void splitAt(DerivedBlock& split);
	void splitBelow(DerivedBlock& split);
	void extend(Cuboid& cuboid);
};
