#include "blocks.h"
#include "../area.h"
#include "../blockFeature.h"
bool Blocks::blockFeature_contains(BlockIndex block, const BlockFeatureType& blockFeatureType) const
{
	for(const BlockFeature& blockFeature : m_features.at(block))
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return true;
	return false;
}
// Can return nullptr.
BlockFeature* Blocks::blockFeature_at(BlockIndex block, const BlockFeatureType& blockFeatureType)
{
	for(BlockFeature& blockFeature : m_features.at(block))
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return &blockFeature;
	return nullptr;
}
const BlockFeature* Blocks::blockFeature_atConst(BlockIndex block, const BlockFeatureType& blockFeatureType) const
{
	return const_cast<Blocks&>(*this).blockFeature_at(block, blockFeatureType);
}
void Blocks::blockFeature_remove(BlockIndex block, const BlockFeatureType& blockFeatureType)
{
	assert(!solid_is(block));
	std::erase_if(m_features.at(block), [&](BlockFeature& bf) { return bf.blockFeatureType == &blockFeatureType; });
	m_area.m_hasTerrainFacades.update(block);
	m_area.m_hasActors.m_opacityFacade.update(block);
}
void Blocks::blockFeature_removeAll(BlockIndex block)
{
	assert(!solid_is(block));
	m_features.at(block).clear();
	m_area.m_hasTerrainFacades.update(block);
	m_area.m_hasActors.m_opacityFacade.update(block);
}
void Blocks::blockFeature_construct(BlockIndex block, const BlockFeatureType& blockFeatureType, const MaterialType& materialType)
{
	assert(!solid_is(block));
	if(plant_exists(block))
	{
		assert(!plant_get(block).m_plantSpecies.isTree);
		plant_erase(block);
	}
	m_features.at(block).emplace_back(&blockFeatureType, &materialType, false);
	if((blockFeatureType == BlockFeatureType::floor || blockFeatureType == BlockFeatureType::hatch) && !materialType.transparent)
	{
		m_area.m_hasActors.m_visionCuboids.blockFloorIsSometimesOpaque(block);
		setBelowExposedToSky(block);
	}
	else if(blockFeatureType == BlockFeatureType::door && !materialType.transparent)
	{
		m_area.m_hasActors.m_visionCuboids.blockIsSometimesOpaque(block);
		setBelowNotExposedToSky(block);
	}
	m_area.m_hasTerrainFacades.update(block);
	m_area.m_hasActors.m_opacityFacade.update(block);
}
void Blocks::blockFeature_hew(BlockIndex block, const BlockFeatureType& blockFeatureType)
{
	assert(!solid_is(block));
	m_features.at(block).emplace_back(&blockFeatureType, &solid_get(block), true);
	solid_setNot(block);
	m_area.m_hasActors.m_opacityFacade.update(block);
}
void Blocks::blockFeature_setTemperature(BlockIndex block, Temperature temperature)
{
	for(BlockFeature& feature : m_features.at(block))
		if(feature.materialType->burnData != nullptr && temperature > feature.materialType->burnData->ignitionTemperature)
			m_area.m_fires.ignite(block, *feature.materialType);
}
void Blocks::blockFeature_lock(BlockIndex block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.locked = true;
	m_area.m_hasTerrainFacades.update(block);
}
void Blocks::blockFeature_unlock(BlockIndex block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.locked = false;
	m_area.m_hasTerrainFacades.update(block);
}
void Blocks::blockFeature_close(BlockIndex block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.closed = true;
	m_area.m_hasActors.m_opacityFacade.update(block);
}
void Blocks::blockFeature_open(BlockIndex block, const BlockFeatureType& blockFeatueType)
{
	assert(blockFeature_contains(block, blockFeatueType));
	BlockFeature& blockFeature = *blockFeature_at(block, blockFeatueType);
	blockFeature.closed = false;
	m_area.m_hasActors.m_opacityFacade.update(block);
}
// Blocks entrance from all angles, does not include floor and hatch which only block from below.
bool Blocks::blockFeature_blocksEntrance(BlockIndex block) const
{
	for(const BlockFeature& blockFeature : m_features.at(block))
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::fortification || blockFeature.blockFeatureType == &BlockFeatureType::floodGate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::door && blockFeature.locked)
		  )
			return true;
	}
	return false;
}
bool Blocks::blockFeature_canStandIn(BlockIndex block) const
{
	for(const BlockFeature& blockFeature : m_features.at(block))
		if(blockFeature.blockFeatureType->canStandIn)
			return true;
	return false;
}
bool Blocks::blockFeature_canStandAbove(BlockIndex block) const
{
	for(const BlockFeature& blockFeature : m_features.at(block))
		if(blockFeature.blockFeatureType->canStandAbove)
			return true;
	return false;
}
bool Blocks::blockFeature_isSupport(BlockIndex block) const
{
	for(const BlockFeature& blockFeature : m_features.at(block))
		if(blockFeature.blockFeatureType->isSupportAgainstCaveIn)
			return true;
	return false;
}
bool Blocks::blockFeature_canEnterFromBelow(BlockIndex block) const
{
	for(const BlockFeature& blockFeature : m_features.at(block))
		if(blockFeature.blockFeatureType == &BlockFeatureType::floor || blockFeature.blockFeatureType == &BlockFeatureType::floorGrate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
bool Blocks::blockFeature_canEnterFromAbove([[maybe_unused]] BlockIndex block, BlockIndex from) const
{
	assert(shape_anythingCanEnterEver(block));
	for(const BlockFeature& blockFeature : m_features.at(from))
		if(blockFeature.blockFeatureType == &BlockFeatureType::floor || blockFeature.blockFeatureType == &BlockFeatureType::floorGrate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
const MaterialType* Blocks::blockFeature_getMaterialType(BlockIndex block) const
{
	if(m_features.at(block).empty())
		return nullptr;
	return m_features.at(block)[0].materialType;
}
bool Blocks::blockFeature_empty(BlockIndex index) const
{
	return m_features.at(index).empty();
}
