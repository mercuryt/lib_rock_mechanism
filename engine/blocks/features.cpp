#include "blocks.h"
#include "../area/area.h"
#include "../blockFeature.h"
#include "../actors/actors.h"
#include "../plants.h"
#include "../reference.h"
#include "../types.h"
bool Blocks::blockFeature_contains(const BlockIndex& block, const BlockFeatureTypeId& blockFeatureType) const
{
	return m_features[block].contains(blockFeatureType);
}
// Can return nullptr.
BlockFeature* Blocks::blockFeature_at(const BlockIndex& block, const BlockFeatureTypeId& blockFeatureType)
{
	return m_features[block].maybeGet(blockFeatureType);
}
const BlockFeature* Blocks::blockFeature_atConst(const BlockIndex& block, const BlockFeatureTypeId& blockFeatureType) const
{
	return const_cast<Blocks&>(*this).blockFeature_at(block, blockFeatureType);
}
void Blocks::blockFeature_remove(const BlockIndex& block, const BlockFeatureTypeId& blockFeatureType)
{
	assert(!solid_is(block));
	bool transmitedTemperaturePreviously = temperature_transmits(block);
	m_features[block].remove(blockFeatureType);
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
	if(!transmitedTemperaturePreviously && temperature_transmits(block))
		m_area.m_exteriorPortals.onBlockCanTransmitTemperature(m_area, block);
}
void Blocks::blockFeature_removeAll(const BlockIndex& block)
{
	assert(!solid_is(block));
	bool transmitedTemperaturePreviously = temperature_transmits(block);
	m_features[block].clear();
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
	if(!transmitedTemperaturePreviously && temperature_transmits(block))
		m_area.m_exteriorPortals.onBlockCanTransmitTemperature(m_area, block);
}
void Blocks::blockFeature_construct(const BlockIndex& block, const BlockFeatureTypeId& blockFeatureType, const MaterialTypeId& materialType)
{
	assert(!solid_is(block));
	bool transmitedTemperaturePreviously = temperature_transmits(block);
	Plants& plants = m_area.getPlants();
	if(plant_exists(block))
	{
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant_get(block))));
		plant_erase(block);
	}
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_features[block].insert(blockFeatureType, materialType, false, true, false);
	if((blockFeatureType == BlockFeatureTypeId::Floor || blockFeatureType == BlockFeatureTypeId::Hatch) && !MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.blockFloorIsOpaque(block);
		m_exposedToSky.unset(m_area, block);
	}
	else if(blockFeatureType == BlockFeatureTypeId::Door && !MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.blockIsOpaque(block);
		m_exposedToSky.unset(m_area, block);
	}
	m_area.m_opacityFacade.update(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
	if(transmitedTemperaturePreviously && !temperature_transmits(block))
		m_area.m_exteriorPortals.onBlockCanNotTransmitTemperature(m_area, block);
}
void Blocks::blockFeature_hew(const BlockIndex& block, const BlockFeatureTypeId& blockFeatureType)
{
	assert(solid_is(block));
	m_features[block].insert(blockFeatureType, solid_get(block), true, true, false);
	// Solid_setNot will handle calling m_exteriorPortals.onBlockCanTransmitTemperature.
	// TODO: There is no support for hewing doors, hatches or flaps. This is ok because those things can't be hewn. Could be fixed anyway?
	const BlockIndex& above = getBlockAbove(block);
	auto actorsCopy = actor_getAll(above);
	for(const ActorIndex& actor : actorsCopy)
		m_area.getActors().tryToMoveSoAsNotOccuping(actor, above);
	solid_setNot(block);
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_setTemperature(const BlockIndex& block, const Temperature& temperature)
{
	for(BlockFeature& feature : m_features[block])
	{
		if(MaterialType::getIgnitionTemperature(feature.materialType).exists() &&
			temperature > MaterialType::getIgnitionTemperature(feature.materialType)
		)
			m_area.m_fires.ignite(block, feature.materialType);
	}
}
void Blocks::blockFeature_setAll(const BlockIndex& index, BlockFeatureSet& features)
{
	// Unused.
	assert(m_features[index].empty());
	m_features[index] = features;
	m_area.m_opacityFacade.update(index);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(index);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(index);
}
void Blocks::blockFeature_setAllMoveDynamic(const BlockIndex& index, BlockFeatureSet&& features)
{
	assert(m_features[index].empty());
	auto& movedFeatures = m_features[index] = std::move(features);
	m_area.m_opacityFacade.update(index);
	if(movedFeatures.isOpaque())
		m_area.m_visionCuboids.blockIsOpaque(index);
	else if(movedFeatures.floorIsOpaque())
		m_area.m_visionCuboids.blockFloorIsOpaque(index);
}
void Blocks::blockFeature_lock(const BlockIndex& block, const BlockFeatureTypeId& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.locked = true;
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_unlock(const BlockIndex& block, const BlockFeatureTypeId& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.locked = false;
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_close(const BlockIndex& block, const BlockFeatureTypeId& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	assert(!blockFeature.closed);
	blockFeature.closed = true;
	m_area.m_opacityFacade.update(block);
	m_area.m_exteriorPortals.onBlockCanNotTransmitTemperature(m_area, block);
	if(!MaterialType::getTransparent(blockFeature.materialType))
	{
		if(blockFeatueType == BlockFeatureTypeId::Hatch)
			m_area.m_visionCuboids.blockFloorIsOpaque(block);
		else
			m_area.m_visionCuboids.blockIsOpaque(block);
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	}
}
void Blocks::blockFeature_open(const BlockIndex& block, const BlockFeatureTypeId& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.closed = false;
	m_area.m_opacityFacade.update(block);
	m_area.m_exteriorPortals.onBlockCanTransmitTemperature(m_area, block);
	if(!MaterialType::getTransparent(blockFeature.materialType))
	{
		if(blockFeatueType == BlockFeatureTypeId::Hatch)
			m_area.m_visionCuboids.blockFloorIsTransparent(block);
		else
			m_area.m_visionCuboids.blockIsTransparent(block);
		m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	}
}
// Blocks entrance from all angles, does not include floor and hatch which only block from below.
bool Blocks::blockFeature_blocksEntrance(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
	{
		if(blockFeature.blockFeatureTypeId == BlockFeatureTypeId::Fortification || blockFeature.blockFeatureTypeId == BlockFeatureTypeId::FloodGate ||
				(blockFeature.blockFeatureTypeId == BlockFeatureTypeId::Door && blockFeature.locked)
		  )
			return true;
	}
	return false;
}
bool Blocks::blockFeature_canStandIn(const BlockIndex& block) const
{
	return m_features[block].canStandIn();
}
bool Blocks::blockFeature_canStandAbove(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(BlockFeatureType::byId(blockFeature.blockFeatureTypeId).canStandAbove)
			return true;
	return false;
}
bool Blocks::blockFeature_isSupport(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(BlockFeatureType::byId(blockFeature.blockFeatureTypeId).isSupportAgainstCaveIn)
			return true;
	return false;
}
bool Blocks::blockFeature_canEnterFromBelow(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureTypeId == BlockFeatureTypeId::Floor || blockFeature.blockFeatureTypeId == BlockFeatureTypeId::FloorGrate ||
				(blockFeature.blockFeatureTypeId == BlockFeatureTypeId::Hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
bool Blocks::blockFeature_canEnterFromAbove([[maybe_unused]] const BlockIndex& block, const BlockIndex& from) const
{
	assert(shape_anythingCanEnterEver(block));
	for(const BlockFeature& blockFeature : m_features[from])
		if(blockFeature.blockFeatureTypeId == BlockFeatureTypeId::Floor || blockFeature.blockFeatureTypeId == BlockFeatureTypeId::FloorGrate ||
				(blockFeature.blockFeatureTypeId == BlockFeatureTypeId::Hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
MaterialTypeId Blocks::blockFeature_getMaterialType(const BlockIndex& block) const
{
	if(m_features[block].empty())
		return MaterialTypeId::null();
	return m_features[block].front().materialType;
}
bool Blocks::blockFeature_empty(const BlockIndex& index) const
{
	return m_features[index].empty();
}
bool Blocks::blockFeature_multiTileCanEnterAtNonZeroZOffset(const BlockIndex& index) const
{
	for(const BlockFeature& blockFeature : m_features[index])
		if(BlockFeatureType::byId(blockFeature.blockFeatureTypeId).blocksMultiTileShapesIfNotAtZeroZOffset)
			return false;
	return true;
}