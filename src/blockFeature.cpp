#include "blockFeature.h"
#include "block.h"
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
	return const_cast<const HasBlockFeatures&>(*this).at(blockFeatureType);
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
	m_features.emplace_back(blockFeatureType, materialType, false);
	m_block.m_hasShapes.clearCache();
}
void HasBlockFeatures::hew(const BlockFeatureType& blockFeatureType)
{
	assert(m_block.isSolid());
	m_features.emplace_back(blockFeatureType, m_block.getSolidMaterial(), true);
	m_block.m_hasShapes.clearCache();
}
bool HasBlockFeatures::blocksEntrance() const
{
	for(const BlockFeature& blockFeature : m_features)
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::fortification || blockFeature.blockFeatureType == &BlockFeatureType::floodGate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::door && blockFeature.locked)
		  )
			return false;
	}
	return true;
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
