/*
 * A cuboid of space where every block has line of sight to every other block.
 * Used to skip line of sight calculations.
 * Stored in Area::m_visionCuboids. Deleted durring DerivedArea::writeStep if m_destroy is true.
 */
#pragma once
#include <cstdint>
#include "cuboid.h"

class Area;

class VisionCuboid
{
public:
	static void setup(Area& area);
	static void clearDestroyed(Area& area);
	static void BlockIsNeverOpaque(Block& block);
	static void BlockIsSometimesOpaque(Block& block);
	static void BlockFloorIsNeverOpaque(Block& block);
	static void BlockFloorIsSometimesOpaque(Block& block);
	static VisionCuboid* getTargetToCombineWith(const Cuboid& cuboid);

	Cuboid m_cuboid;
	bool m_destroy;

	VisionCuboid(Cuboid& cuboid);
	bool canSeeInto(const Cuboid& cuboid) const;
	bool canCombineWith(const Cuboid& cuboid) const;
	void splitAt(Block& split);
	void splitBelow(Block& split);
	void extend(Cuboid& cuboid);
};
