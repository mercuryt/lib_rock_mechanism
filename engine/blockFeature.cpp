#include "blockFeature.h"
#include "block.h"
#include "area.h"
// Name, hewn, blocks entrance, lockable, stand in, stand above
BlockFeatureType BlockFeatureType::floor = {"floor", false, false, false, true, false, false};
BlockFeatureType BlockFeatureType::door = {"door", false, false, true, false, false, false};
BlockFeatureType BlockFeatureType::flap = {"flap", false, false, false, false, false, false};
BlockFeatureType BlockFeatureType::hatch = {"hatch", false, false, true, true, false, false};
BlockFeatureType BlockFeatureType::stairs = {"stairs", true,  false, false, true, true, false};
BlockFeatureType BlockFeatureType::floodGate = {"floodGate", true, true, false, false, true, false};
BlockFeatureType BlockFeatureType::floorGrate = {"floorGrate", false, false, false, true, false, false};
BlockFeatureType BlockFeatureType::ramp = {"ramp", true, false, false, true, true, false};
BlockFeatureType BlockFeatureType::fortification = {"fortification", true, true, false, false, true, true};
bool HasBlockFeatures::contains(const BlockFeatureType& blockFeatureType) const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return true;
	return false;
}
// Can return nullptr.
BlockFeature* HasBlockFeatures::at(const BlockFeatureType& blockFeatureType)
{
	for(BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return &blockFeature;
	return nullptr;
}
const BlockFeature* HasBlockFeatures::at(const BlockFeatureType& blockFeatureType) const
{
	return const_cast<HasBlockFeatures&>(*this).at(blockFeatureType);
}
void HasBlockFeatures::remove(const BlockFeatureType& blockFeatureType)
{
	assert(!m_block.isSolid());
	std::erase_if(m_features, [&](BlockFeature& bf) { return bf.blockFeatureType == &blockFeatureType; });
	m_block.m_hasShapes.clearCache();
}
void HasBlockFeatures::construct(const BlockFeatureType& blockFeatureType, const MaterialType& materialType)
{
	assert(!m_block.isSolid());
	if(m_block.m_hasPlant.exists())
	{
		assert(!m_block.m_hasPlant.get().m_plantSpecies.isTree);
		m_block.m_hasPlant.get().die();
	}
	m_features.emplace_back(blockFeatureType, materialType, false);
	m_block.m_hasShapes.clearCache();
	if((blockFeatureType == BlockFeatureType::floor || blockFeatureType == BlockFeatureType::hatch) && !materialType.transparent)
	{
		if(m_block.m_area->m_visionCuboidsActive)
			VisionCuboid::BlockFloorIsSometimesOpaque(m_block);
		m_block.setBelowNotExposedToSky();
	}
}
void HasBlockFeatures::hew(const BlockFeatureType& blockFeatureType)
{
	assert(m_block.isSolid());
	m_features.emplace_back(blockFeatureType, m_block.getSolidMaterial(), true);
	m_block.setNotSolid();
	m_block.m_hasShapes.clearCache();
}
void HasBlockFeatures::setTemperature(Temperature temperature)
{
	for(BlockFeature& feature : m_features)
		if(feature.materialType->burnData != nullptr && temperature > feature.materialType->burnData->ignitionTemperature)
			m_block.m_area->m_fires.ignite(m_block, *feature.materialType);
}
// Blocks entrance from all angles, does not include floor and hatch which only block from below.
bool HasBlockFeatures::blocksEntrance() const
{
	for(const BlockFeature& blockFeature : m_features)
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::fortification || blockFeature.blockFeatureType == &BlockFeatureType::floodGate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::door && blockFeature.locked)
		  )
			return true;
	}
	return false;
}
bool HasBlockFeatures::canStandIn() const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType->canStandIn)
			return true;
	return false;
}
bool HasBlockFeatures::canStandAbove() const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType->canStandAbove)
			return true;
	return false;
}
bool HasBlockFeatures::isSupport() const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType->isSupportAgainstCaveIn)
			return true;
	return false;
}
bool HasBlockFeatures::canEnterFromBelow() const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType == &BlockFeatureType::floor || blockFeature.blockFeatureType == &BlockFeatureType::floorGrate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
bool HasBlockFeatures::canEnterFromAbove(const Block& from) const
{
	for(const BlockFeature& blockFeature : from.m_hasBlockFeatures.m_features)
		if(blockFeature.blockFeatureType == &BlockFeatureType::floor || blockFeature.blockFeatureType == &BlockFeatureType::floorGrate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::hatch && blockFeature.locked)
		  )
			return false;
	return true;
}
