#pragma once
#include "cuboidSet.h"

namespace cuboidSetHelper
{
	inline std::vector<CuboidSet> splitIntoTouchingGroups(const CuboidSet& input)
	{
		std::vector<CuboidSet> output;
		std::vector<Cuboid> boundry;
		for(Cuboid inputCuboid : input)
		{
			bool added = false;
			int addedTo{-1};
			int outputCount = output.size();
			for(int i{0}; i < outputCount; ++i)
			{
				if(boundry[i].isTouching(inputCuboid) && output[i].isTouching(inputCuboid))
				{
					if(!added)
					{
						addedTo = i;
						added = true;
						output[i].add(inputCuboid);
						boundry[i].maybeExpand(inputCuboid);
					}
					else
					{
						// merge with previously added to set.
						output[addedTo].add(output[i]);
						boundry[addedTo].maybeExpand(boundry[i]);
						// Clear output[i] as signal to erase.
						output[i].clear();
						boundry[i].clear();
					}
				}
			}
			if(!added)
			{
				// inputCuboid does not touch any existing group.
				output.emplace_back(inputCuboid.toSet());
				boundry.push_back(inputCuboid);
			}
			std::erase_if(output, [](const CuboidSet& cuboids) { return cuboids.empty(); });
		}
		return output;
	}
};