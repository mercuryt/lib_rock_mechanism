#include "blocks.h"
#include "../area.h"
#include "../materialType.h"
#include "../types.h"
#include "../blockFeature.h"
#include "../fluidType.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"

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
	m_area.m_opacityFacade.update(index);
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
	m_constructed.unset(index);
	fluid_onBlockSetNotSolid(index);
	m_area.m_opacityFacade.update(index);
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
		m_area.m_visionCuboids.blockIsOpaque(index);
}
void Blocks::solid_setNot(const BlockIndex& index)
{
	solid_setNotShared(index);
	m_area.m_visionCuboids.blockIsTransparent(index);
}
void Blocks::solid_setCuboid(const Cuboid& cuboid, const MaterialTypeId& materialType, bool constructed)
{
	for(const BlockIndex& index : cuboid.getView(*this))
		solid_setShared(index, materialType, constructed);
	if(!MaterialType::getTransparent(materialType))
		m_area.m_visionCuboids.cuboidIsOpaque(cuboid);
}
void Blocks::solid_setNotCuboid(const Cuboid& cuboid)
{
	for(const BlockIndex& index : cuboid.getView(*this))
		solid_setNotShared(index);
	m_area.m_visionCuboids.cuboidIsTransparent(cuboid);
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
	return MaterialType::getDensity(m_materialType[index]) * Volume::create(Config::maxBlockVolume.get());
}