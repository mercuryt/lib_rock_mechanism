#include "space.h"
#include "../definitions/moveType.h"
#include "../rtreeHelpers/rtreeHelpers.h"
#include "../area/area.h"

CuboidSet Space::move_queryPathable(const Cuboid cuboid, const MoveTypeId moveType) const
{
	CuboidSet output;
	if(MoveType::getFly(moveType))
		output.add(cuboid);
	else if(MoveType::getSurface(moveType))
	{
		int climb = MoveType::getClimb(moveType);
		// Add on top of solid cuboids.
		Cuboid cuboidShiftedDownOne = cuboid.maybeShifted(Facing6::Below, {1});
		m_solid.queryForEachCuboid(cuboidShiftedDownOne, [&](const Cuboid solidCuboid){
			if(solidCuboid.m_high.z() != m_sizeZ - 1)
			{
				Cuboid above = solidCuboid.intersection(cuboidShiftedDownOne).getFaceAbove();
				above.shift(Facing6::Above, {1});
				output.add(above);
			}
		});
		Cuboid cuboidExtendedDownOne = cuboid;
		if(cuboid.m_low.z() != 0)
			cuboidExtendedDownOne.m_low.setZ(cuboid.m_low.z() - 1);
		// Add features, both on and in.
		m_features.queryForEachWithCuboids(cuboidExtendedDownOne, [&](const Cuboid featureCuboid, const PointFeature feature){
			const PointFeatureType& type = PointFeatureType::byId(feature.pointFeatureType);
			if(featureCuboid.m_high.z() != m_sizeZ - 1 && type.canStandAbove)
			{
				Cuboid above = featureCuboid.intersection(cuboidExtendedDownOne).getFaceAbove();
				above.shift(Facing6::Above, {1});
				output.maybeAdd(above);
			}
			// For stand in we exclude feature cuboids that don't intersect cuboid because we extended down one, outside of the area meant to be queried by cuboid.
			if(type.canStandIn && featureCuboid.intersects(cuboid))
				output.maybeAdd(featureCuboid.intersection(cuboid));
		});
		// Climb1 allows for diagonal vertical movement between pathable cuboids but does not make any otherwise unpathable space pathable.
		// It is handled in move_cuboidCanBeEnteredFrom.
		if(climb == 2)
		{
			// Add any point which is horizontally adjacent to a wall.
			Cuboid expanded = cuboid.inflated({1});
			CuboidSet walls = m_solid.queryGetAllCuboids(expanded);
			m_features.queryForEachWithCuboids(expanded, [&](const Cuboid featureCuboid, const PointFeature feature){
				if(PointFeatureType::byId(feature.pointFeatureType).blocksEntrance)
					walls.add(featureCuboid);
			});
			Cuboid spaceBoundry = boundry();
			for(const Cuboid wall : walls)
			{
				Cuboid adjacentToWall = wall.inflatedHorizontal({1}).intersection(spaceBoundry);
				output.maybeAdd(adjacentToWall);
			}
		}
	}
	SmallMap<FluidTypeId, CollisionVolume>& swimData = MoveType::getSwim(moveType);
	if(!swimData.empty())
	{
		const bool breathless = MoveType::getBreathless(moveType);
		const SmallSet<FluidTypeId>& breathable = MoveType::getBreathableFluids(moveType);
		m_fluid.queryForEachWithCuboids(cuboid, [&](const Cuboid fluidCuboid, const FluidData fluidData){
			auto found = swimData.find(fluidData.type);
			int64_t volumeOfFluidInCuboid = m_area.m_hasFluidGroups.byId(fluidData.group).getVolume(CuboidSet::create(cuboid));
			CollisionVolume perBlockVolume{(CollisionVolumeWidth)(volumeOfFluidInCuboid / cuboid.intersection(fluidCuboid).volume())};
			bool canEnter = found != swimData.end() && found->second <= perBlockVolume;
			if(canEnter)
			{
				if(breathable.contains(fluidData.type))
					// MoveType can breath this fluid so add the whole cuboid.
					output.maybeAdd(fluidCuboid);
				else
				{
					// MoveType cannot breath this fluid, add only those parts on the top with air above them.
					if(fluidCuboid.m_high.z() == m_sizeZ - 1)
						return;
					Cuboid above = fluidCuboid.getFaceAbove();
					above.shift({0, 0, 1}, {1});
					CuboidSet enterable = CuboidSet::create(above);
					m_solid.queryRemove(enterable);
					if(enterable.empty())
						return;
					m_features.queryRemoveWithCondition(enterable, [](const PointFeature feature){
						return feature.pointFeatureType == PointFeatureTypeId::Floor || (feature.pointFeatureType == PointFeatureTypeId::Hatch && feature.isLocked());
					});
					if(enterable.empty())
						return;
					enterable.shift({0, 0, -1}, {1});
					output.maybeAddAll(enterable);
					CuboidSet toRemove = CuboidSet::create(fluidCuboid);
					toRemove.maybeRemoveAll(enterable);
					output.maybeRemoveAll(toRemove);
				}
			}
			else if(!breathless)
				output.maybeRemove(fluidCuboid);
		});
	}
	else if(!MoveType::getBreathless(moveType) && !output.empty())
		// No swiming ability.
		m_fluid.queryRemove(output);
	auto [fluidType, depth] = MoveType::getFloating(moveType);
	if(fluidType.exists())
	{
		assert(swimData.empty());
		assert(!MoveType::getSurface(moveType));
		assert(!MoveType::getFly(moveType));

		// Collect cuboids with the same fluid type because otherwise mergable cuboids might be kept seperate due to different volumes.
		CuboidSet fluidContainingCuboids;
		m_fluid.queryForEachWithCuboids(cuboid, [cuboid, fluidType, &fluidContainingCuboids](Cuboid fluidCuboid, FluidData fluidData){
			//TODO: filter too shallow to count?
			if(fluidData.type == fluidType)
				fluidContainingCuboids.add(fluidCuboid.intersection(cuboid));
		});
		// Filter out too shallow.
		for(Cuboid fluidCuboid : fluidContainingCuboids)
		{
			Distance zLevel = fluidCuboid.m_high.z() - depth;
			if(fluidCuboid.sizeZ() < depth)
			{
				CuboidSet filtered = CuboidSet::create(fluidCuboid);
				//Find any gaps in the fluid between the bottom of fluid cuboid and zLevel.
				CuboidSet containsFluid = m_fluid.queryGetAllCuboids(fluidCuboid);
				CuboidSet doesNotContainFluid = containsFluid.inverted();
				CuboidSet doesNotContainFluidProjectedOnToZLevel;
				for(Cuboid doesNotContainCuboid : doesNotContainFluid)
				{
					doesNotContainCuboid.m_high.setZ(zLevel);
					doesNotContainCuboid.m_low.setZ(zLevel);
					doesNotContainFluidProjectedOnToZLevel.maybeAdd(doesNotContainCuboid);
				}
				CuboidSet containsFluidProjectedOnToZLevel;
				for(Cuboid containsCuboid : containsFluid)
				{
					containsCuboid.m_high.setZ(zLevel);
					containsCuboid.m_low.setZ(zLevel);
					containsFluidProjectedOnToZLevel.maybeAdd(containsCuboid);
				}
				if(!containsFluidProjectedOnToZLevel.empty())
					containsFluidProjectedOnToZLevel.maybeRemoveAll(doesNotContainFluidProjectedOnToZLevel);
				if(!containsFluidProjectedOnToZLevel.empty())
					output.addAll(containsFluidProjectedOnToZLevel);
			}
			else
			{
				fluidCuboid.m_high.setZ(zLevel);
				fluidCuboid.m_low.setZ(zLevel);
				output.add(fluidCuboid);
			}
		}
	}
	output = output.intersection(cuboid);
	// Filter output for unpathable space.
	if(!output.empty())
	{
		CuboidSet unpathable = m_solid.queryGetIntersection(output);
		m_features.queryForEachWithCuboids(output, [&](const Cuboid featureCuboid, const PointFeature feature){
			if(feature.blocksEntrance())
				unpathable.maybeAdd(featureCuboid.intersection(cuboid));
		});
		if(!unpathable.empty())
			m_dynamic.queryForEach(unpathable.boundry(), [&unpathable](Cuboid dynamicCuboid){ unpathable.maybeRemove(dynamicCuboid); });
		output.maybeRemoveAll(unpathable);
	}
	return output;
}
SmallSet<Cuboid> Space::move_splitCuboidByPartitions(const Cuboid cuboid) const
{
	SmallSet<Cuboid> output;
	CuboidSet partitions = m_features.queryGetAllCuboidsWithCondition(cuboid, [&](const PointFeature feature){
		return feature.blocksVerticalTravel();
	});
	// Get horizontal slices.
	SmallSet<Cuboid> partitionsSeperated;
	for(const Cuboid partitionCuboid : partitions)
		partitionsSeperated.insertAll(partitionCuboid.sliceAtEachZ());
	// Sort by z, descendnig.
	partitionsSeperated.sortUnaryDescending([&](const Cuboid partitionedCuboid){ return partitionedCuboid.m_high.z(); });
	CuboidSet remainder = CuboidSet::create(cuboid);
	// For each partition remove the space above it from remainder and add it to output.
	for(const Cuboid partitionCuboid : partitionsSeperated)
	{
		Cuboid query = partitionCuboid;
		query.m_high.setZ(cuboid.m_high.z());
		output.insertAll(remainder.intersection(query).m_cuboids);
		remainder.maybeRemove(query);
	}
	// Add remainder.
	output.insertAll(remainder.m_cuboids);
	return output;
}
bool Space::move_cuboidCanBeEnteredFrom(const Cuboid from, const Cuboid to, const MoveTypeId moveType) const
{
	bool overlapZ = to.m_low.z() <= from.m_high.z() && to.m_high.z() >= from.m_low.z();
	if(overlapZ)
		// Horizontal movement is always allowed.
		return true;
	// Floating can only move horizontally.
	// No need to check in short range pathing because enterable cuboids for float are always flat.
	if(MoveType::getFloating(moveType).first.exists())
		return false;
	bool overlapX = to.m_low.x() <= from.m_high.x() && to.m_high.x() >= from.m_low.x();
	bool overlapY = to.m_low.y() <= from.m_high.y() && to.m_high.y() >= from.m_low.y();
	Cuboid portalAbove;
	bool movingUp = to.m_low.z() > from.m_high.z();
	if(movingUp)
	{
		if(MoveType::getSurface(moveType) && MoveType::getClimb(moveType) == 0)
		{
			if((!overlapX || !overlapY) && !move_canSwimInAny(to.inflated({1}).intersection(from), moveType))
				// This is an edge or corner connection.
				return false;
		}
		portalAbove = to.inflated({{1}}).intersection(from);
		portalAbove.shift({0,0,1}, {1});
		portalAbove.m_low.setZ(portalAbove.m_high.z());
	}
	else // Moving down.
	{
		if(MoveType::getSurface(moveType) && (MoveType::getClimb(moveType) == 0 && !MoveType::getJumpDown(moveType)))
		{
			if((!overlapX || !overlapY) && !move_canSwimInAny(from.inflated({1}).intersection(to), moveType))
				// This is an edge or corner connection.
				return false;
		}
		portalAbove = from.inflated({{1}}).intersection(to);
		portalAbove.shift({0,0,1}, {1});
		portalAbove.m_low.setZ(portalAbove.m_high.z());
	}
	CuboidSet aboveFiltered = CuboidSet::create(portalAbove);
	m_solid.queryRemove(aboveFiltered);
	if(aboveFiltered.empty())
		return false;
	return RTreeHelpers::queryAnyNotWithConditionNonOverlaping(m_features, aboveFiltered, [&](const PointFeature feature){ return feature.blocksVerticalTravel(); });
}
bool Space::move_partitionExistsBetween(const Cuboid a, const Cuboid b) const
{
	if(a.overlapZ(b))
		return false;
	Cuboid high = (a.m_low.z() > b.m_high.z()) ? a : b;
	return m_features.queryAnyWithCondition(high.getFaceBelow(), [](const PointFeature feature) { return feature.blocksVerticalTravel(); });
}
bool Space::move_canSwimInAny(const Cuboid cuboid, const MoveTypeId moveType) const
{
	const auto& swimData = MoveType::getSwim(moveType);
	bool result = false;
	m_fluid.queryForEach(cuboid, [&](const FluidData data){
		if(result)
			return;
		auto found = swimData.find(data.type);
		if(found == swimData.end())
			return;
		int64_t volumeOfFluidInCuboid = m_area.m_hasFluidGroups.byId(data.group).getVolume(CuboidSet::create(cuboid));
		if(found->second.get() <= volumeOfFluidInCuboid / cuboid.getFaceBelow().volume())
			result = true;
	});
	return result;
}
void Space::move_removeUnenterableFrom(CuboidSet& cuboids) const
{
	m_solid.queryRemove(cuboids);
	if(!cuboids.empty())
		m_features.queryRemoveWithCondition(cuboids, [](PointFeature feature){ return feature.blocksEntrance(); });
}