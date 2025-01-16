#include "blocks.h"
#include "../area.h"
#include "../blockFeature.h"
#include "plants.h"
#include "reference.h"
#include "types.h"
bool Blocks::blockFeature_contains(const BlockIndex& block, const BlockFeatureType& blockFeatureType) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return true;
	return false;
}
// Can return nullptr.
BlockFeature* Blocks::blockFeature_at(const BlockIndex& block, const BlockFeatureType& blockFeatureType)
{
	for(BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return &blockFeature;
	return nullptr;
}
const BlockFeature* Blocks::blockFeature_atConst(const BlockIndex& block, const BlockFeatureType& blockFeatureType) const
{
	return const_cast<Blocks&>(*this).blockFeature_at(block, blockFeatureType);
}
void Blocks::blockFeature_remove(const BlockIndex& block, const BlockFeatureType& blockFeatureType)
{
	assert(!solid_is(block));
	std::erase_if(m_features[block], [&](BlockFeature& bf) { return bf.blockFeatureType == &blockFeatureType; });
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_removeAll(const BlockIndex& block)
{
	assert(!solid_is(block));
	m_features[block].clear();
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_construct(const BlockIndex& block, const BlockFeatureType& blockFeatureType, const MaterialTypeId& materialType)
{
	Plants& plants = m_area.getPlants();
	assert(!solid_is(block));
	if(plant_exists(block))
	{
		assert(!PlantSpecies::getIsTree(plants.getSpecies(plant_get(block))));
		plant_erase(block);
	}
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_features[block].emplace_back(&blockFeatureType, materialType, false);
	if((blockFeatureType == BlockFeatureType::floor || blockFeatureType == BlockFeatureType::hatch) && !MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.blockFloorIsSometimesOpaque(m_area, block);
		setBelowExposedToSky(block);
	}
	else if(blockFeatureType == BlockFeatureType::door && !MaterialType::getTransparent(materialType))
	{
		m_area.m_visionCuboids.blockIsSometimesOpaque(m_area, block);
		setBelowNotExposedToSky(block);
	}
	m_area.m_opacityFacade.update(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_hew(const BlockIndex& block, const BlockFeatureType& blockFeatureType)
{
	assert(solid_is(block));
	m_features[block].emplace_back(&blockFeatureType, solid_get(block), true);
	solid_setNot(block);
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_setTemperature(const BlockIndex& block, const Temperature& temperature)
{
	for(BlockFeature& feature : m_features[block])
	{
		if(MaterialType::getIgnitionTemperature(m_materialType[block]).exists() &&
			temperature > MaterialType::getIgnitionTemperature(feature.materialType)
		)
			m_area.m_fires.ignite(block, feature.materialType);
	}
}
void Blocks::blockFeature_lock(const BlockIndex& block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.locked = true;
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_unlock(const BlockIndex& block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.locked = false;
	m_area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
}
void Blocks::blockFeature_close(const BlockIndex& block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.closed = true;
	m_area.m_opacityFacade.update(block);
}
void Blocks::blockFeature_open(const BlockIndex& block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.closed = false;
	m_area.m_opacityFacade.update(block);
	m_area.m_visionRequests.maybeGenerateRequestsForAllWithLineOfSightTo(block);
}
// Blocks entrance from all angles, does not include floor and hatch which only block from below.
bool Blocks::blockFeature_blocksEntrance(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::fortification || blockFeature.blockFeatureType == &BlockFeatureType::floodGate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::door && blockFeature.locked)
		  )
			return true;
	}
	return false;
}
bool Blocks::blockFeature_canStandIn(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureType->canStandIn)
			return true;
	return false;
}
bool Blocks::blockFeature_canStandAbove(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureType->canStandAbove)
			return true;
	return false;
}
bool Blocks::blockFeature_isSupport(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureType->isSupportAgainstCaveIn)
			return true;
	return false;
}
bool Blocks::blockFeature_canEnterFromBelow(const BlockIndex& block) const
{
	for(const BlockFeature& blockFeature : m_features[block])
		if(blockFeature.blockFeatureType == &BlockFeatureType::floor || blockFeature.blockFeatureType == &BlockFeatureType::floorGrate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
bool Blocks::blockFeature_canEnterFromAbove([[maybe_unused]] const BlockIndex& block, const BlockIndex& from) const
{
	assert(shape_anythingCanEnterEver(block));
	for(const BlockFeature& blockFeature : m_features[from])
		if(blockFeature.blockFeatureType == &BlockFeatureType::floor || blockFeature.blockFeatureType == &BlockFeatureType::floorGrate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
MaterialTypeId Blocks::blockFeature_getMaterialType(const BlockIndex& block) const
{
	if(m_features[block].empty())
		return MaterialTypeId::null();
	return m_features[block][0].materialType;
}
bool Blocks::blockFeature_empty(const BlockIndex& index) const
{
	return m_features[index].empty();
}
