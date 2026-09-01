#pragma once
#include "../numericTypes/idTypes.h"
#include "../dataStructures/smallMap.h"
#include "../geometry/cuboidSet.h"
class Area;

namespace buildBuilding
{
	struct Paramaters
	{
		std::vector<Cuboid> rooms;
		SmallMap<PointFeatureTypeId, CuboidSet> features;
		MaterialTypeId foundationMaterial;
		MaterialTypeId externalWallMaterial;
		MaterialTypeId internalWallMaterial;
		MaterialTypeId roofMaterial;
		MaterialTypeId floorMaterial;
	};
	void execute(Area& area, Paramaters&& paramaters);
	void mudHut(Area& area, Cuboid location, Facing4 facing);
}