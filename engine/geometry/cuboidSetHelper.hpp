#include "cuboidSetHelper.h"
#include "cuboidSet.h"

// Perform a binary classification over a cuboidSet.
CuboidSet cuboidSetHelper::query(const CuboidSet& input, auto&& conditionCuboid, auto&& conditionPoint)
{
	CuboidSet output;
	std::vector<Cuboid> openList;
	openList.push_back(input.boundry());
	static int maximumVolumeForNarrowPhase = 20;
	while(!openList.empty())
	{
		Cuboid candidate = openList.back();
		openList.pop_back();
		std::optional<bool> result = conditionCuboid(candidate);
		if(!result.has_value())
		{
			if(candidate.volume() <= maximumVolumeForNarrowPhase)
			{
				for(Cuboid cuboid : input.intersection(candidate))
					for(Point3D point : cuboid)
						if(conditionPoint(point))
							output.add(point);
			}
			else
			{
				std::pair<Cuboid, Cuboid> pair = candidate.splitByLongestDimension();
				openList.push_back(pair.first);
				openList.push_back(pair.second);
			}
		}
		else
		{
			if(result.value())
				output.add(candidate.intersection(input));
			// If there is a negitive result do nothing, the candidate is discarded.
		}
	}
	return output;
}

std::pair<CuboidSet, CuboidSet> cuboidSetHelper::queryReturnTrueAndFalse(const CuboidSet& input, auto&& conditionCuboid, auto&& conditionPoint)
{
	CuboidSet trueOutput;
	CuboidSet falseOutput;
	std::vector<Cuboid> openList;
	openList.push_back(input.boundry());
	static int maximumVolumeForNarrowPhase = 20;
	while(!openList.empty())
	{
		Cuboid candidate = openList.back();
		openList.pop_back();
		std::optional<bool> result = conditionCuboid(candidate);
		if(!result.has_value())
		{
			if(candidate.volume() <= maximumVolumeForNarrowPhase)
			{
				for(Cuboid cuboid : input.intersection(candidate))
					for(Point3D point : cuboid)
						if(conditionPoint(point))
							trueOutput.add(point);
						else
							falseOutput.add(point);
			}
			else
			{
				std::pair<Cuboid, Cuboid> pair = candidate.splitByLongestDimension();
				openList.push_back(pair.first);
				openList.push_back(pair.second);
			}
		}
		else
		{
			if(result.value())
				trueOutput.add(candidate.intersection(input));
			else
				falseOutput.add(candidate.intersection(input));
		}
	}
	return {trueOutput, falseOutput};
}
