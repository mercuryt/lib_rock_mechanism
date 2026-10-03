#pragma once
#include "../geometry/cuboidSet.h"
#include "../dataStructures/rtreeBoolean.h"
#include "world.h"

// TODO: This is slow.
std::vector<Point3D> findPathWorld(const RTreeBoolean& enterable, Point3D begin, Point3D end)
{
	assert(begin != end);
	CuboidSet closedList;
	std::vector<std::pair<Point3D, Point3D>> history;
	std::vector<Point3D> openList;
	openList.push_back(begin);
	closedList.add(begin);
	while(!openList.empty())
	{
		auto found = std::ranges::min_element(openList, {}, [end](Point3D candidate) { return candidate.distanceTo(end); });
		Point3D current = *found;
		(*found) = openList.back();
		openList.pop_back();
		if(current == end)
		{
			std::vector<Point3D> output;
			output.push_back(end);
			while(true)
			{
				current = std::ranges::find_if(history, [current](std::pair<Point3D, Point3D> pair) -> bool { return pair.first == current; })->second;
				if(current == begin)
					return output;
				output.push_back(current);
			}
		}
		CuboidSet candidates = enterable.queryGetIntersection(current.inflated());
		candidates.maybeRemoveAll(closedList);
		closedList.add(candidates);
		for(Cuboid cuboid : candidates)
			for(Point3D candidate : cuboid)
			{
				history.emplace_back(candidate, current);
				openList.push_back(candidate);
			}
	}
	// No roue found.
	return {};
}
CuboidSet makeEnterableWorld(World& world, MoveTypeId moveType, Cuboid cuboid)
{
	CuboidSet output;
	if(MoveType::getFly(moveType))
		output.add(cuboid);
	else if(MoveType::getSurface(moveType))
	{
		Cuboid cuboidShiftedDownOne = cuboid.maybeShifted(Facing6::Below, {1});
		world.m_solid.queryForEachCuboid(cuboidShiftedDownOne, [&](const Cuboid solidCuboid){
			if(solidCuboid.m_high.z() != cuboid.sizeZ() - 1)
			{
				Cuboid above = solidCuboid.intersection(cuboidShiftedDownOne).getFaceAbove();
				above.shift(Facing6::Above, {1});
				output.add(above);
			}
		});
	}
	// Swimming.
	SmallMap<FluidTypeId, CollisionVolume>& swimData = MoveType::getSwim(moveType);
	if(!swimData.empty())
	{
		const bool breathless = MoveType::getBreathless(moveType);
		const SmallSet<FluidTypeId>& breathable = MoveType::getBreathableFluids(moveType);
		world.m_fluid.queryForEachWithCuboids(cuboid, [&](Cuboid fluidCuboid, FluidTypeId fluidType){
			auto found = swimData.find(fluidType);
			if(found != swimData.end())
			{
				if(breathable.contains(fluidType))
					// MoveType can breath this fluid so add the whole cuboid and the face above.
					output.maybeAdd(fluidCuboid.inflatedDirection(Facing6::Above));
				else
					// Cannot breath, can only swim on surface.
					output.maybeAdd(fluidCuboid.getFaceAbove().shifted(Facing6::Above));
			}
			else if(!breathless || !breathable.contains(fluidType))
				// Cannot swim in or breath this fluid.
				output.maybeRemove(fluidCuboid);
		});
	}
	else if(!MoveType::getBreathless(moveType) && !output.empty())
		// No swiming ability.
		world.m_fluid.queryRemove(output);
	// Floating.
	auto [fluidType, depth] = MoveType::getFloating(moveType);
	if(fluidType.exists())
	{
		assert(swimData.empty());
		assert(!MoveType::getSurface(moveType));
		assert(!MoveType::getFly(moveType));
		for(Cuboid fluidCuboid : world.m_fluid.queryGetAllCuboidsWithCondition(cuboid, [fluidType](FluidTypeId t){ return t == fluidType; }))
			output.maybeAdd(fluidCuboid.getFaceAbove().shifted(Facing6::Above));
	}
	// Filter output for unpathable space.
	if(!output.empty())
	{
		CuboidSet unpathable = world.m_solid.queryGetIntersection(output);
		output.maybeRemoveAll(unpathable);
	}
	return output;
}