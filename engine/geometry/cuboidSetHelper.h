#pragma once
#include "cuboidSet.h"
class Random;

namespace cuboidSetHelper
{
	[[nodiscard]] std::vector<CuboidSet> splitIntoTouchingGroups(const CuboidSet& input);
	[[nodiscard]] std::vector<int> makeFlowField(const CuboidSet& input, std::vector<int> start, std::vector<int> end);
	struct RandomClusterParamaters
	{
		CuboidSet source;
		Cuboid area;
		int count;
		float maxRatioOfLongToShortDimension;
		Distance maxDimension;
		Distance minDimension;
	};
	[[nodiscard]] CuboidSet randomCluster(Random& random, RandomClusterParamaters paramaters);
	[[nodiscard]] CuboidSet query(const CuboidSet& input, auto&& conditionCuboid, auto&& conditionPoint);
	[[nodiscard]] std::pair<CuboidSet, CuboidSet> queryReturnTrueAndFalse(const CuboidSet& input, auto&& conditionCuboid, auto&& conditionPoint);
	[[nodiscard]] Point3D nearestPointToWithConiditon(const CuboidSet& input, Point3D start, auto&& condition);
};