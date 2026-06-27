#include "space.h"
#include "../fluidType.h"
#include "../fluid/fluidGroup.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../deserializationMemo.h"
#include "../numericTypes/types.h"
#include "../geometry/point3D.h"
#include "../geometry/cuboidSetHelper.h"

void Space::fluid_flowInto(const CuboidSet& cuboids, FluidTypeId fluidType, FluidGroup& group)
{
	m_fluid.insert(cuboids, FluidData(group.m_id, fluidType));
	// Update temperature.
	if(!group.m_aboveGround && m_exposedToSky.check(cuboids))
	{
		group.m_aboveGround = true;
		if(FluidType::getFreezesInto(fluidType).exists())
			m_area.m_hasTemperature.onFluidEnters(m_area, cuboids, fluidType, group.m_id);
	}
	m_area.m_hasPaths.update(m_area, cuboids.inflated({1}));
	floating_maybeFloatUp(cuboids);
	fluid_maybeRecordFluidOnDeck(cuboids);
}
void Space::fluid_flowOutFrom(const CuboidSet& cuboids, FluidTypeId type)
{
	m_fluid.removeWithCondition(cuboids, [type](FluidData fluid) { return fluid.type == type; });
	m_area.m_hasPaths.update(m_area, cuboids.inflated({1}));
	floating_maybeSink(cuboids);
	fluid_maybeEraseFluidOnDeck(cuboids);
}
void Space::fluid_setGroupId(const CuboidSet& shape, FluidTypeId type, FluidGroupId group)
{
	auto action = [group](FluidData& fluidData){ fluidData.group = group; };
	auto condition = [type](const FluidData& data){ return data.type == type; };
	constexpr decltype(m_fluid)::UpdateActionConfig queryConfig{.allowNotChanged = true};
	m_fluid.updateActionWithCondition<queryConfig>(shape, action, condition);
}
void Space::fluid_removeAllFilledWithDensityEqualOrGreaterThenFrom(CuboidSet& cuboids, FluidTypeId fluidType) const
{
	Density density = FluidType::getDensity(fluidType);
	SmallSet<FluidGroupId> groupIds;
	m_fluid.queryForEach(cuboids.boundry(), [&groupIds, density](FluidData fluid){
		if(fluid.density > density)
			groupIds.maybeInsert(fluid.group);
	});
	for(FluidGroupId id : groupIds)
	{
		FluidGroup& group = m_area.m_hasFluidGroups.byId(id);
		group.removeFullFrom(cuboids);
		if(cuboids.empty())
			break;
	}
}
void Space::fluid_add(const CuboidSet& shape, int64_t volume, FluidTypeId fluidType)
{
	m_area.m_hasFluidGroups.createGroup(shape, volume, fluidType);
	FluidGroup& group = m_area.m_hasFluidGroups.m_groups.back();
	fluid_flowInto(shape, fluidType, group);
}
void Space::fluid_remove(const CuboidSet& shape, int64_t volume, FluidTypeId fluidType)
{
	SmallSet<FluidGroupId> toDestroy;
	for(FluidGroup* group : fluid_getGroupsWithType(shape, fluidType))
	{
		// partially drained groups are resolved during next fluid step, fully drained ones are destroyed immediately.
		if(group->m_volume > volume)
		{
			group->m_volume -= volume;
			group->m_stable = false;
		}
		else
		{
			volume -= group->m_volume;
			toDestroy.insert(group->m_id);
		}
	}
	for(FluidGroupId id : toDestroy)
	{
		FluidGroup& group = m_area.m_hasFluidGroups.byId(id);
		fluid_flowOutFrom(group.m_occupied, group.m_fluidType);
		m_area.m_hasPaths.update(m_area, group.m_occupied);
		m_area.m_hasFluidGroups.destroyGroup(id);
	}
}
SmallSet<FluidGroup*> Space::fluid_getGroups(const CuboidSet& shape)
{
	SmallSet<FluidGroupId> ids;
	m_fluid.queryForEach(shape, [&ids](FluidData data){ ids.maybeInsert(data.group); });
	SmallSet<FluidGroup*> output;
	output.reserve(ids.size());
	for(FluidGroupId id : ids)
		output.insert(&m_area.m_hasFluidGroups.byId(id));
	return output;
}
SmallSet<FluidGroup*> Space::fluid_getGroupsWithType(const CuboidSet& shape, FluidTypeId fluidType)
{
	SmallSet<FluidGroupId> ids;
	m_fluid.queryForEach(shape, [&ids, fluidType](FluidData data){ if(data.type == fluidType) ids.maybeInsert(data.group); });
	SmallSet<FluidGroup*> output;
	output.reserve(ids.size());
	for(FluidGroupId id : ids)
		output.insert(&m_area.m_hasFluidGroups.byId(id));
	return output;
}
CuboidSet Space::fluid_queryGetCuboids(const Cuboid shape) const
{
	return m_fluid.queryGetAllCuboids(shape);
}
FluidGroup* Space::fluid_getGroup(const Point3D point, const FluidTypeId fluidType) const
{
	FluidData data = m_fluid.queryGetOneWithCondition(point, [fluidType](FluidData fluid) { return fluid.type == fluidType; });
	if(data.empty())
		return nullptr;
	return &m_area.m_hasFluidGroups.byId(data.group);
}
CollisionVolume Space::fluid_volumeOfTypeContains(const Point3D point, const FluidTypeId fluidType) const
{
	FluidGroup* group = fluid_getGroup(point, fluidType);
	if(group == nullptr)
		return {0};
	bool isTrailingPoint = group->m_flowingUp ?
		point.z() == group->m_lowZ :
		point.z() == group->m_highZ;
	if(isTrailingPoint)
		return CollisionVolume::create(group->trailingLevelFluidVolumePerPoint());
	else
		return Config::maxPointVolume;
}
void Space::fluid_onSetSolid(const CuboidSet& cuboids)
{
	SmallSet<FluidGroupId> displaced;
	for(FluidGroup* group : fluid_getGroups(cuboids))
	{
		if(cuboids.contains(group->m_occupied))
			displaced.insert(group->m_id);
		else
		{
			CuboidSet removed = group->m_occupied.intersection(cuboids);
			group->m_occupied.remove(removed);
			group->m_stable = false;
			group->m_noLongerOccupied.add(removed);
		}
	}
	CuboidSet candidates = cuboids.inflated({1});
	candidates.removeAll(cuboids);
	solid_removeAllFrom(candidates);
	if(!candidates.empty())
	{
		Point3D destinination = candidates[0].m_low;
		for(FluidGroupId id : displaced)
		{
			FluidGroup& group = m_area.m_hasFluidGroups.byId(id);
			group.m_stable = false;
			group.m_occupied.clear();
			group.m_occupied.add(destinination);
		}
	}
	else
	{
		// Nowhere to put displaced, destroy it.
		for(FluidGroupId id : displaced)
			m_area.m_hasFluidGroups.destroyGroup(id);
	}
	m_fluid.maybeRemove(cuboids);
}
void Space::fluid_onSetNotSolid(const CuboidSet& cuboids)
{
	for(FluidGroup* group : fluid_getGroups(cuboids.inflated({1})))
	group->m_stable = false;
}
CollisionVolume Space::fluid_getTotalVolume(const Point3D point) const
{
	SmallSet<FluidGroupId> ids;
	m_fluid.queryForEach(point, [&ids](FluidData data){ ids.maybeInsert(data.group); });
	CollisionVolume output{0};
	for(FluidGroupId id : ids)
	{
		FluidGroup& group = m_area.m_hasFluidGroups.byId(id);
		if(
			(group.m_flowingUp && group.m_lowZ != point.z()) ||
			(!group.m_flowingUp && group.m_highZ != point.z())
		)
			return Config::maxPointVolume;
		else
			output += group.trailingLevelFluidVolume();
	}
	return output;
}
const MapWithCuboidKeys<std::pair<FluidTypeId, int64_t>> Space::fluid_getWithCuboidsAndRemoveAll(const CuboidSet& cuboids)
{
	MapWithCuboidKeys<std::pair<FluidTypeId, int64_t>> output;
	for(const auto& [fluidCuboid, fluidData] : m_fluid.queryGetAllWithCuboids(cuboids))
	{
		FluidGroup& group = m_area.m_hasFluidGroups.byId(fluidData.group);
		int64_t volume = (group.m_volume * fluidCuboid.volume()) / group.m_occupied.volume();
		group.m_volume -= volume;
		group.m_stable = false;
		output.insert(fluidCuboid, {fluidData.type, volume});
	}
	m_fluid.removeAll(cuboids);
	return output;
}
template<typename ShapeT>
bool Space::fluid_contains(ShapeT shape, const FluidTypeId fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	return m_fluid.queryAnyWithCondition(shape, condition);
}
template bool Space::fluid_contains<Point3D>(Point3D shape, const FluidTypeId fluidType) const;
template bool Space::fluid_contains<Cuboid>(Cuboid shape, const FluidTypeId fluidType) const;
bool Space::fluid_contains(const CuboidSet& shape, const FluidTypeId fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	return m_fluid.queryAnyWithCondition(shape, condition);
}
template<typename ShapeT>
Point3D Space::fluid_containsPoint(ShapeT&& shape, const FluidTypeId fluidType) const
{
	const auto condition = [fluidType](const FluidData& data) { return data.type == fluidType; };
	const Cuboid result = m_fluid.queryGetOneCuboidWithCondition(shape, condition);
	if(result.empty())
		return Point3D::null();
	return result.intersection(shape).m_high;

}
template Point3D Space::fluid_containsPoint<Point3D>(Point3D&& shape, const FluidTypeId fluidType) const;
template Point3D Space::fluid_containsPoint<const Point3D&>(const Point3D& shape, const FluidTypeId fluidType) const;
template Point3D Space::fluid_containsPoint<Cuboid>(Cuboid&& shape, const FluidTypeId fluidType) const;
template Point3D Space::fluid_containsPoint<const Cuboid&>(const Cuboid& shape, const FluidTypeId fluidType) const;
void Space::fluid_maybeRecordFluidOnDeck(const CuboidSet& points)
{
	const DeckId deckId = m_area.m_decks.queryDeckId(points);
	if(deckId.exists())
	{
		const ActorOrItemIndex isOnDeckOf = m_area.m_decks.getForId(deckId);
		assert(isOnDeckOf.isItem());
		const ItemIndex item = isOnDeckOf.getItem();
		m_area.getItems().onDeck_recordPointsContainingFluid(item, points);
	}
}
void Space::fluid_maybeEraseFluidOnDeck(const CuboidSet& points)
{
	const DeckId deckId = m_area.m_decks.queryDeckId(points);
	if(deckId.exists())
	{
		const ActorOrItemIndex isOnDeckOf = m_area.m_decks.getForId(deckId);
		assert(isOnDeckOf.isItem());
		const ItemIndex item = isOnDeckOf.getItem();
		m_area.getItems().onDeck_erasePointsContainingFluid(item, points);
	}
}
bool Space::fluid_shapeIsMostlySurroundedByFluidOfTypeAtDistanceAboveLocationWithFacing(const ShapeId shape, const FluidTypeId fluidType, const Distance distance, const Point3D location, const Facing4 facing) const
{
	const CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(shape, *this, location, facing);
	const Distance floatLevel = location.z() + distance;
	// Truncate toOccupy above the waterline and then flatten it there to create a horizontal cross-section.
	const Point3D truncateAboveHigh(Distance::max(), Distance::max(), Distance::max());
	const Point3D truncateAboveLow(Distance::min(), Distance::min(), floatLevel + 1);
	const Cuboid toRemoveFromOccupied = Cuboid::create(truncateAboveHigh, truncateAboveLow);
	CuboidSet toOccupyTruncatedAndFlattened = toOccupy;
	toOccupyTruncatedAndFlattened.maybeRemove(toRemoveFromOccupied);
	toOccupyTruncatedAndFlattened = toOccupyTruncatedAndFlattened.flattened(floatLevel);
	const CuboidSet toOccupyAdjacentAtZLevel = toOccupyTruncatedAndFlattened.adjacentSlicedAtZ(floatLevel);
	auto condition = [&](const FluidData& data) { return data.type == fluidType; };
	const CuboidSet adjacentAtZLevelWithFluidType = m_fluid.queryGetAllCuboidsWithCondition(toOccupyAdjacentAtZLevel, condition).intersection(toOccupyAdjacentAtZLevel);
	float ratio = (float)toOccupyAdjacentAtZLevel.volume() / (float)adjacentAtZLevelWithFluidType.volume();
	return ratio >= Config::minimumRatioAdjacentSpaceContainingFluidToFloat;
}