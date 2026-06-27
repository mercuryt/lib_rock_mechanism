#pragma once

#include "../numericTypes/types.h"
#include "../geometry/cuboidSet.h"
#include <cmath>
/*
	Algorithims for working with rtrees.
*/
namespace RTreeHelpers
{
	// Lambda query has signature query(Cuboid)->CuboidSet;
	CuboidSet findCuboidSetWithMaxVolumeAndMaxRangeStartingFromCuboid(const int maxVolume, const int maxRange, const Cuboid start, auto&& query)
	{
		Distance range{(DistanceWidth)(maxRange / 2)};
		auto getCuboidsWithRange = [&](const Distance thisRange) -> CuboidSet {
			const Cuboid cuboid = start.inflated(thisRange);
			CuboidSet output = query(cuboid);
			// Start may not be part of output when the source blocks propigatiton, for example a wall on fire blocks heat.
			output.maybeAdd(start);
			output = output.adjacentRecursive(start);
			return output;
		};
		CuboidSet output = getCuboidsWithRange(range);
		int volume = output.volume();
		int previousVolume = 0;
		while(volume < maxVolume && range < maxRange)
		{
			if((float)volume / (float)(maxVolume) < 0.5)
				range = Distance::create((float)range.get() * 1.5f);
			else
				++range;
			CuboidSet next = getCuboidsWithRange(range);
			int nextVolume = next.volume();
			if(nextVolume == maxVolume)
				return next;
			if(nextVolume > maxVolume || nextVolume == previousVolume)
				// Either this step added too much or none. Either way continue with previous value in output.
				break;
			else
			{
				output = std::move(next);
				previousVolume = volume;
			}
		}
		while(volume > maxVolume && range > 1)
		{
			--range;
			output = getCuboidsWithRange(range);
			volume = output.volume();
		}
		return output;
	}
	CuboidSet findCuboidSetWithMaxVolumeAndMaxRangeStartingFromPoint(const int maxVolume, const int maxRange, const Point3D start, auto&& query)
	{
		return findCuboidSetWithMaxVolumeAndMaxRangeStartingFromCuboid(maxVolume, maxRange, Cuboid::create(start, start), std::move(query));
	}
	template<typename T>
	CuboidSet getAdjacentWithConditionRecursive(auto& rtree, const auto start, auto&& condition)
	{
		Cuboid startCuboid = rtree.queryGetOneCuboidWithCondition(start, condition);
		CuboidSet output;
		if(startCuboid.empty())
			return output;
		output.add(startCuboid);
		CuboidSet openList;
		openList.add(startCuboid);
		constexpr bool unary = std::is_invocable_v<decltype(condition), T>;
		while(!openList.empty())
		{
			Cuboid candidate = openList.back();
			openList.popBack();
			// Get adjacent.
			candidate.inflate({1});
			rtree.queryForEachWithCuboids(candidate, [&](const Cuboid cuboid, const auto& value){
				bool result;
				if constexpr(unary)
					result = condition(value);
				else
					result = condition(cuboid, value);
				// Output is also closed list.
				if(result && !output.contains(cuboid))
				{
					output.add(cuboid);
					openList.add(cuboid);
				}
			});
		}
		return output;
	}
	// Assumes that cuboids which satisfy condition do not overlap eachother.
	CuboidSet deleteAdjacentWithConditionRecursive(auto& rtree, const auto start, auto&& condition)
	{
		SmallSet<Cuboid> openList;
		SmallSet<Cuboid> closedList;
		rtree.queryForEachWithCuboids(start.inflated({1}), [&](Cuboid cuboid, const auto& value){
			if(condition(value))
			{
				openList.insert(cuboid);
				closedList.insert(cuboid);
			}
		});
		while(!openList.empty())
		{
			Cuboid candidate = openList.back();
			openList.popBack();
			// Get adjacent.
			candidate.inflate({1});
			rtree.queryForEachWithCuboids(candidate, [&](const Cuboid cuboid, const auto& value){
				if(condition(value) && !closedList.contains(cuboid))
				{
					openList.insert(cuboid);
					closedList.insert(cuboid);
				}
			});
		}
		CuboidSet toRemove = CuboidSet::create(closedList);
		rtree.removeWithCondition(toRemove, [&](const auto& value){ return condition(value); });
		return toRemove;
	}
	bool queryAnyNotWithConditionNonOverlaping(const auto& rtree, const auto& shape, auto&& condition)
	{
		int volume = shape.volume();
		rtree.queryForEachWithCuboids(shape, [&](const Cuboid cuboid, const auto& value){
			if(condition(value))
				volume -= shape.intersection(cuboid).volume();
		});
		// A volume less then zero means overlaping results.
		assert(volume >= 0);
		return volume != 0;
	}
};