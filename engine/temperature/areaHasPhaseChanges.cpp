#include "areaHasPhaseChanges.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../items/items.h"
#include "../config/physics.h"
#include "../geometry/cuboidSetHelper.hpp"
void AreaHasPhaseChanges::doStep(Area& area)
{
	Step step = area.m_simulation.m_step;
	// Performs at most one melt and one freeze per step.
	// TODO: Despite not being on the same tick operations are clusterld directly after step % meltStepFrequency == 0.
	int remainder = (step % Config::Physics::phaseChangeFrequency).get();
	if(remainder < m_melting.size())
		doMelt(area, m_melting.m_data[remainder].first, m_melting.m_data[remainder].second);
	if(remainder < m_freezing.size())
		doFreeze(area, m_freezing.m_data[remainder].first, m_freezing.m_data[remainder].second);
}
void AreaHasPhaseChanges::doMelt(Area& area, MaterialTypeId materialType, CuboidSet cuboidSet)
{
	Random& random = area.m_simulation.m_random;
	Space& space = area.getSpace();
	Items& items = area.getItems();
	int64_t volumeToMelt = std::max(1, (int)(cuboidSet.volume() * Config::Physics::unitsOfFluidToMeltPerPointVolume));
	FluidTypeId fluidType = MaterialType::getMeltsInto(materialType);
	for(int i{0}; i < volumeToMelt; ++i)
	{
		if(cuboidSet.empty())
			return;
		Point3D point = random.getInCuboidSet(cuboidSet);
		if(space.solid_isAny(point))
		{
			cuboidSet.remove(point);
			space.solid_setNot(point);
			space.item_addChunksAndPiles(point.toSet(), Config::maxPointVolume - 1, materialType);
			space.fluid_add(point.toSet(), 1, fluidType);
		}
		else
		{
			auto condition = [materialType](PointFeature feature){ return feature.materialType == materialType; };
			auto itemCondition = [materialType, &items](ItemIndex item){ return items.getMaterialType(item) == materialType; };
			PointFeature pointFeature = space.pointFeature_queryGetOne(point, condition);
			if(pointFeature.exists())
			{
				space.pointFeature_remove(point, pointFeature.pointFeatureType);
				space.item_addChunksAndPiles(point.toSet(), Config::Physics::volumeOfFluidToGenerateWhenAPointFeatureMelts - 1, materialType);
				// If there are no other features with this type and there are also no items remove it from the event.
				space.fluid_add(point.toSet(), 1, fluidType);
				if(
					!space.pointFeature_queryAnyWithCondition(point, condition) &&
					!space.item_queryAnyWithCondition(point, itemCondition)
				)
					cuboidSet.remove(point);
			}
			else
			{
				CollisionVolume smallestVolume{CollisionVolume::max()};
				ItemIndex smallestItem;
				space.item_queryForEach(point, [&smallestVolume, &smallestItem, &items, materialType](ItemIndex item){
					if(items.getMaterialType(item) != materialType)
						return;
					CollisionVolume itemVolume{Shape::getTotalCollisionVolume(items.getShape(item))};
					if(smallestVolume > itemVolume)
					{
						smallestVolume = itemVolume;
						smallestItem = item;
					}
				});
				if(smallestItem.empty())
				{
					cuboidSet.remove(point);
				}
				else
				{
					CuboidSet occupied = items.getOccupied(smallestItem);
					items.remove(smallestItem);
					space.fluid_add(occupied, smallestVolume.get(), fluidType);
				}
			}
		}
	}
}
void AreaHasPhaseChanges::doFreeze(Area& area, FluidTypeId fluidType, CuboidSet cuboidSet)
{
	Random& random = area.m_simulation.m_random;
	Space& space = area.getSpace();
	Items& items = area.getItems();
	int64_t volumeToFreeze = std::max(1, (int)(cuboidSet.volume() * Config::Physics::unitsOfFluidToFreezePerPointVolume));
	MaterialTypeId materialType = FluidType::getFreezesInto(fluidType);
	for(int i{0}; i < volumeToFreeze; ++i)
	{
		Point3D point = random.getInCuboidSet(cuboidSet);
		CollisionVolume volumeOfItems{0};
		SmallSet<ItemIndex> itemsPresent;
		space.item_queryForEach(point, [materialType, &volumeOfItems, &items, &itemsPresent](ItemIndex item){
			if(items.getMaterialType(item) != materialType)
				return;
			volumeOfItems += Shape::getTotalCollisionVolume(items.getShape(item));
			itemsPresent.insert(item);
		});
		space.fluid_remove(point.toSet(), 1, fluidType);
		if(volumeOfItems >= Config::maxPointVolume - 1)
		{
			// Freeze solid.
			CollisionVolume volumeDestroyed{0};
			for(ItemIndex item : itemsPresent)
			{
				CollisionVolume itemVolume = Shape::getTotalCollisionVolume(items.getShape(item));
				Quantity toDestroy;
				if(itemVolume.get() + volumeDestroyed.get() > Config::maxPointVolume)
				{
					// More then enough to fill point.
					toDestroy = {((Config::maxPointVolume - volumeDestroyed - 1) / itemVolume).get() + 1};
					CollisionVolume remainder = volumeDestroyed + (toDestroy.get() * itemVolume.get()) - Config::maxPointVolume - 1;
					space.item_addChunksAndPiles(point.toSet(), remainder, materialType);
				}
				else
					toDestroy = items.getQuantity(item);
				volumeDestroyed += ItemType::getFullDisplacement(items.getItemType(item)).toCollisionVolume() * toDestroy;
				items.removeQuantity(item, toDestroy);
				if(volumeDestroyed >= Config::maxPointVolume - 1)
					break;
			}
			// Any items not destroyed will be distributed.
			space.solid_setAll(point.toSet(), materialType, false);
		}
		else
		{
			// Consolidate pile into chunk if possible.
			static ItemTypeId pileType{ItemType::byName("pile")};
			static ItemTypeId chunkType{ItemType::byName("chunk")};
			static CollisionVolume chunkVolume{Shape::getTotalCollisionVolume(ItemType::getShape(chunkType))};
			static CollisionVolume pileVolume{Shape::getTotalCollisionVolume(ItemType::getShape(pileType))};
			static Quantity pilesPerChunk{chunkVolume.get() / pileVolume.get()};
			ItemIndex pile = space.item_getOneWithCondition(point, [materialType, &items](ItemIndex item){
				return items.getMaterialType(item) == materialType && items.getItemType(item) == pileType;
			});
			Quantity pileQuantity{pile.empty() ? Quantity{0} : items.getQuantity(pile)};
			if(pileQuantity + 1 >= pilesPerChunk)
			{
				items.removeQuantity(pile, pilesPerChunk - 1);
				space.item_addGeneric(point, chunkType, materialType, {1});
			}
			else
				// Can't consolidate, add pile instead.
				space.item_addGeneric(point, pileType, materialType, {1});
		}
		if(!space.fluid_contains(point, fluidType))
		{
			cuboidSet.remove(point);
			if(cuboidSet.empty())
				return;
		}
	}
}

void AreaHasPhaseChanges::onFluidEnters(Area& area, FluidTypeId fluidType, const CuboidSet& cuboidSet)
{
	Temperature freezingPoint = FluidType::getFreezingPoint(fluidType);
	if(freezingPoint.empty())
		return;
	auto cuboidCondition = [freezingPoint, &area](Cuboid cuboid) -> std::optional<bool> {
		auto [upper, lower] = area.m_hasTemperature.upperAndLowerBounds(area, cuboid);
		// If the highest temperature that could exist in the cuboid is less then or equal to freezing point then the whole cuboid freezes.
		if(upper <= freezingPoint)
			return {true};
		// If the lowest temperature that could exist in the cuboid is greater then or equal to freezing point then none of the cuboid freezes.
		if(lower >= freezingPoint)
			return {false};
		// Some but not all of the cuboid freezes.
		return std::optional<bool>{};
	};
	auto pointCondition = [freezingPoint, &area](Point3D point) -> bool {
		return freezingPoint >= area.m_hasTemperature.get(area, point);
	};
	CuboidSet toFreeze = cuboidSetHelper::query(cuboidSet, cuboidCondition, pointCondition);
	if(toFreeze.exists())
		m_freezing.getOrCreate(fluidType).maybeAdd(toFreeze);
}
void AreaHasPhaseChanges::onFluidExits(FluidTypeId fluidType, const CuboidSet& cuboidSet)
{
	auto found = m_freezing.find(fluidType);
	if(found != m_freezing.end())
		found->second.maybeRemove(cuboidSet);
}
void AreaHasPhaseChanges::onSolidSet(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet)
{
	Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
	if(meltingPoint.empty())
		return;
	auto cuboidCondition = [meltingPoint, &area](Cuboid cuboid) -> std::optional<bool> {
		auto [upper, lower] = area.m_hasTemperature.upperAndLowerBounds(area, cuboid);
		// If the highest temperature that could exist in the cuboid is less then the melting point then none of it melts.
		if(upper < meltingPoint)
			return {false};
		// If the lowest temperature that could exist in the cuboid is greater then or equal to melting point then all of it melts.
		if(lower >= meltingPoint)
			return {true};
		// Some but not all of the cuboid melts.
		return std::optional<bool>{};
	};
	auto pointCondition = [meltingPoint, &area](Point3D point) -> bool {
		return meltingPoint <= area.m_hasTemperature.get(area, point);
	};
	CuboidSet toMelt = cuboidSetHelper::query(cuboidSet, cuboidCondition, pointCondition);
	if(toMelt.exists())
		m_melting.getOrCreate(materialType).maybeAdd(toMelt);
}
void AreaHasPhaseChanges::onSolidSetNot(MaterialTypeId materialType, const CuboidSet& cuboidSet)
{
	auto found = m_melting.find(materialType);
	if(found != m_melting.end())
		found->second.maybeRemove(cuboidSet);
}
void AreaHasPhaseChanges::onFeatureSet(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet)
{
	onSolidSet(area, materialType, cuboidSet);
}
void AreaHasPhaseChanges::onFeatureSetNot(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet)
{
	Items& items = area.getItems();
	CuboidSet occupiedByItems;
	area.getSpace().item_queryForEach(cuboidSet, [materialType, &items, &occupiedByItems](ItemIndex item){
		if(items.getMaterialType(item) == materialType)
			occupiedByItems.maybeAdd(items.getOccupied(item));
	});
	CuboidSet toSetNotMelting = cuboidSet;
	if(occupiedByItems.exists())
		toSetNotMelting.remove(occupiedByItems);
	onSolidSetNot(materialType, toSetNotMelting);
}
void AreaHasPhaseChanges::onItemEnter(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet)
{
	onSolidSet(area, materialType, cuboidSet);
}
void AreaHasPhaseChanges::onItemExit(Area& area, MaterialTypeId materialType, const CuboidSet& cuboidSet)
{
	onFeatureSetNot(area, materialType, cuboidSet);
}
void AreaHasPhaseChanges::setFreezing(FluidTypeId fluid, const CuboidSet& cuboids)
{
	m_freezing.getOrCreate(fluid).maybeAdd(cuboids);
}
void AreaHasPhaseChanges::setNotFreezing(FluidTypeId fluid, const CuboidSet& cuboids)
{
	auto found = m_freezing.find(fluid);
	if(found != m_freezing.end())
		found->second.maybeRemove(cuboids);
}
void AreaHasPhaseChanges::setMelting(MaterialTypeId material, const CuboidSet& cuboids)
{

	m_melting.getOrCreate(material).maybeAdd(cuboids);
}
void AreaHasPhaseChanges::setNotMelting(MaterialTypeId material, const CuboidSet& cuboids)
{
	auto found = m_melting.find(material);
	if(found != m_melting.end())
		found->second.maybeRemove(cuboids);
}