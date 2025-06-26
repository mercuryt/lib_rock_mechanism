#include "blocks.h"
#include "../area/area.h"
#include "../definitions/materialType.h"
#include "../numericTypes/types.h"
#include "../blockFeature.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../portables.hpp"

void Blocks::solid_setShared(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed)
{
	if(m_materialType[index] == materialType)
		return;
	assert(m_itemVolume[index].empty());
	Plants& plants = m_area.getPlants();
	if(m_plants[index].exists())
	{
		PlantIndex plant = m_plants[index];
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant)));
		plants.die(plant);
	}
	if(materialType == m_materialType[index])
		return;
	bool wasEmpty = m_materialType[index].empty();
	if(wasEmpty)
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(index);
	m_materialType[index] = materialType;
	m_constructed.set(index, constructed);
	fluid_onBlockSetSolid(index);
	m_visible.set(index);
	// Blocks below are no longer exposed to sky.
	const BlockIndex& below = getBlockBelow(index);
	if(below.exists() && m_exposedToSky.check(below))
		m_exposedToSky.unset(m_area, below);
	// Record as meltable.
	if(m_exposedToSky.check(index) && MaterialType::canMelt(materialType))
		m_area.m_hasTemperature.addMeltableSolidBlockAboveGround(index);
	// Remove from stockpiles.
	m_area.m_hasStockPiles.removeBlockFromAllFactions(index);
	m_area.m_hasCraftingLocationsAndJobs.maybeRemoveLocation(index);
	if(wasEmpty && m_reservables[index] != nullptr)
		// There are no reservations which can exist on both a solid and not solid block.
		dishonorAllReservations(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	m_area.m_exteriorPortals.onBlockCanNotTransmitTemperature(m_area, index);
}
void Blocks::solid_setNotShared(const BlockIndex& index)
{
	if(!solid_is(index))
		return;
	if(m_exposedToSky.check(index) && MaterialType::canMelt(m_materialType[index]))
		m_area.m_hasTemperature.removeMeltableSolidBlockAboveGround(index);
	m_materialType[index].clear();
	m_constructed.maybeUnset(index);
	fluid_onBlockSetNotSolid(index);
	//TODO: Check if any adjacent are visible first?
	m_visible.set(index);
	setBelowVisible(index);
	// Dishonor all reservations: there are no reservations which can exist on both a solid and not solid block.
	m_reservables[index] = nullptr;
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(index);
	m_area.m_exteriorPortals.onBlockCanTransmitTemperature(m_area, index);
	const BlockIndex& below = getBlockBelow(index);
	if(m_exposedToSky.check(index))
	{
		if(below.exists() && !m_exposedToSky.check(below))
			m_exposedToSky.set(m_area, below);
	}
}
void Blocks::solid_set(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed)
{
	bool wasEmpty = m_materialType[index].empty();
	solid_setShared(index, materialType, constructed);
	// Opacity.
	if(!MaterialType::getTransparent(materialType) && wasEmpty)
	{
		m_area.m_visionCuboids.blockIsOpaque(index);
		m_area.m_opacityFacade.update(m_area, index);
	}
}
void Blocks::solid_setNot(const BlockIndex& index)
{
	bool wasTransparent = MaterialType::getTransparent(solid_get(index));
	solid_setNotShared(index);
	// Vision cuboid.
	if(!wasTransparent)
	{
		m_area.m_visionCuboids.blockIsTransparent(index);
		m_area.m_opacityFacade.update(m_area, index);
	}
	// Gravity.
	const BlockIndex& above = getBlockAbove(index);
	if(above.exists() && shape_anythingCanEnterEver(above))
		maybeContentsFalls(above);
}
void Blocks::solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed)
{
	for(const BlockIndex& index : cuboid.getView(*this))
		solid_setShared(index, materialType, constructed);
	// Vision cuboid.
	if(!MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.cuboidIsOpaque(cuboid);
		m_area.m_opacityFacade.maybeInsertFull(cuboid);
	}
}
void Blocks::solid_setNotCuboid(const Cuboid& cuboid)
{
	for(const BlockIndex& index : cuboid.getView(*this))
		solid_setNotShared(index);
	// Vision cuboid.
	m_area.m_visionCuboids.cuboidIsTransparent(cuboid);
	m_area.m_opacityFacade.maybeRemoveFull(cuboid);
	// Gravity.
	const BlockIndex& aboveHighest = getBlockAbove(getIndex(cuboid.m_highest));
	if(aboveHighest.exists())
	{
		Cuboid aboveCuboid = cuboid.getFace(Facing6::Above);
		aboveCuboid.shift(Facing6::Above, DistanceInBlocks::create(1));
		for(const BlockIndex& above : aboveCuboid.getView(*this))
			maybeContentsFalls(above);
	}
}
MaterialTypeId Blocks::solid_get(const BlockIndex& index) const
{
	return m_materialType[index];
}
bool Blocks::solid_is(const BlockIndex& index) const
{
	return m_materialType[index].exists();
}
Mass Blocks::solid_getMass(const BlockIndex& index) const
{
	assert(solid_is(index));
	return MaterialType::getDensity(m_materialType[index]) * FullDisplacement::create(Config::maxBlockVolume.get());
}
MaterialTypeId Blocks::solid_getHardest(const SmallSet<BlockIndex>& blocks)
{
	MaterialTypeId output;
	uint32_t hardness = 0;
	for(const BlockIndex& block : blocks)
	{
		MaterialTypeId materialType = solid_get(block);
		if(materialType.empty())
			materialType = blockFeature_getMaterialType(block);
		if(materialType.empty())
			continue;
		auto blockHardness = MaterialType::getHardness(materialType);
		if(blockHardness > hardness)
		{
			hardness = blockHardness;
			output = materialType;
		}
	}
	assert(output.exists());
	return output;
}
void Blocks::solid_setDynamic(const BlockIndex& index, const MaterialTypeId& materialType, bool constructed)
{
	assert(m_materialType[index].empty());
	assert(!m_dynamic[index]);
	assert(m_plants[index].empty());
	assert(m_items[index].empty());
	assert(m_actors[index].empty());
	m_materialType[index] = materialType;
	m_dynamic.set(index);
	m_constructed.set(index, constructed);
	if(!MaterialType::getTransparent(materialType))
	{
		m_area.m_opacityFacade.update(m_area, index);
		// TODO: add a multiple blocks at a time version which does this update more efficiently.
		m_area.m_visionCuboids.blockIsOpaque(index);
		// Opacity.
		if(!MaterialType::getTransparent(materialType))
			m_area.m_visionCuboids.blockIsOpaque(index);
	}
}
void Blocks::solid_setNotDynamic(const BlockIndex& index)
{
	assert(m_materialType[index].exists());
	assert(m_dynamic[index]);
	bool wasTransparent = MaterialType::getTransparent(m_materialType[index]);
	m_materialType[index].clear();
	m_dynamic.unset(index);
	m_constructed.maybeUnset(index);
	if(!wasTransparent)
	{
		m_area.m_opacityFacade.update(m_area, index);
		// TODO: add a multiple blocks at a time version which does this update more efficiently.
		m_area.m_visionCuboids.blockIsTransparent(index);
	}
}