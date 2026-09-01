#include "cuboidSetHelper.h"
#include "../random.h"
std::vector<CuboidSet> cuboidSetHelper::splitIntoTouchingGroups(const CuboidSet& input)
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
std::vector<int> cuboidSetHelper::makeFlowField(const CuboidSet& input, std::vector<int> start, std::vector<int> end)
{
	int size = input.size();
	std::vector<int> openList = start;
	openList.reserve(size);
	int openListIndex{0};
	std::vector<int> output(size, INT32_MIN);
	// output acts as closed list.
	for(int i : start)
		output[i] = 0;
	while(openListIndex != (int)openList.size())
	{
		int currentIndex = openList[openListIndex];
		++openListIndex;
		Cuboid current = input[currentIndex];
		for(int i{0}; i != size; ++i)
			if(output[i] == INT32_MIN && input[i].isTouching(current))
			{
				openList.push_back(i);
				output[i] = output[currentIndex] + 1;
			}
	}
	// prune dead ends.
	std::vector<int> indicesSortedByDistance(size);
	std::iota(indicesSortedByDistance.begin(), indicesSortedByDistance.end(), 0);
	std::ranges::sort(indicesSortedByDistance, std::greater{}, [&output](int i){ return output[i]; });
	for(int i : indicesSortedByDistance)
	{
		if(std::ranges::contains(end, i))
			continue;
		bool isDeadEnd = true;
		for(int j{0}; j != size; ++j)
			if(output[i] < output[j] && input[i].isTouching(input[j]))
			{
				isDeadEnd = false;
				break;
			}
		if(isDeadEnd)
			output[i] = INT32_MIN;
	}
	return output;
}
CuboidSet cuboidSetHelper::randomCluster(Random& random, RandomClusterParamaters paramaters)
{
	CuboidSet output;
	for(int i{0}; i < paramaters.count; ++i)
	{
		Point3D center{random.getInCuboidSet(paramaters.source)};
		Distance sizeX{random.getInRange(paramaters.minDimension.get(), paramaters.maxDimension.get())};
		Distance sizeY{random.getInRange(paramaters.minDimension.get(), paramaters.maxDimension.get())};
		Distance sizeZ{random.getInRange(paramaters.minDimension.get(), paramaters.maxDimension.get())};
		Distance lowest{std::min({sizeX.get(), sizeY.get(), sizeZ.get()})};
		Distance max{lowest * paramaters.maxRatioOfLongToShortDimension};
		sizeX = std::min(sizeX, max);
		sizeY = std::min(sizeY, max);
		sizeZ = std::min(sizeZ, max);
		Cuboid cuboid{
			Point3D{center.x() + sizeX / 2, center.y() + sizeY / 2, center.z() + sizeZ / 2},
			Point3D{center.x() - sizeX / 2, center.y() - sizeY / 2, center.z() - sizeZ / 2}
		};
		if(paramaters.area.intersects(cuboid))
		{
			while(output.contains(cuboid))
				cuboid.inflate();
			cuboid = cuboid.intersection(paramaters.area);
			output.add(cuboid);
			paramaters.source.add(cuboid);
		}
	}
	return output;
}