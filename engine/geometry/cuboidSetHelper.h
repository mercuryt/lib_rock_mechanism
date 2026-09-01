#pragma once
#include "cuboidSet.h"
class Random;

namespace cuboidSetHelper
{
	std::vector<CuboidSet> splitIntoTouchingGroups(const CuboidSet& input);
	std::vector<int> makeFlowField(const CuboidSet& input, std::vector<int> start, std::vector<int> end);
	struct RandomClusterParamaters
	{
		CuboidSet source;
		Cuboid area;
		int count;
		float maxRatioOfLongToShortDimension;
		Distance maxDimension;
		Distance minDimension;
	};
	CuboidSet randomCluster(Random& random, RandomClusterParamaters paramaters);
	CuboidSet query(const CuboidSet& input, auto&& conditionCuboid, auto&& conditionPoint);
	std::pair<CuboidSet, CuboidSet> queryReturnTrueAndFalse(const CuboidSet& input, auto&& conditionCuboid, auto&& conditionPoint);
};