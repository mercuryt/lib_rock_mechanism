#include "blockFeature.h"
bool HasBlockFeatures::contains(const BlockFeatureType& blockFeatureType) const
{
	return m_features.end() != std::ranges::find(m_features, blockFeatueType, &BlockFeature::blockFeatureType);
}
// Can return nullptr.
BlockFeature* HasBlockFeatures::at(const BlockFeatureType& blockFeatueType)
{
	auto found = std::ranges::find(m_features, blockFeatueType, &BlockFeature::blockFeatureType);
	if(found == m_features.end())
		return nullptr;
	return &*found;
}
void HasBlockFeatures::remove(const BlockFeatureType& blockFeatueType)
{
	assert(!m_block.isSolid());
	assert(at(blockFeatueType != nullptr));
	std::erase_if(m_features, [&](BlockFeature& bf) { return bf.blockFeatureType == blockFeatueType; });
	m_block.clearMoveCostsCacheForSelfAndAdjacent();
}
void HasBlockFeatures::construct(const BlockFeatureType& blockFeatueType, const MaterialType& materialType)
{
	assert(!m_block.isSolid());
	assert(at(blockFeatueType == nullptr));
	m_features.emplace_back(blockFeatueType, materialType, false);
	m_block.clearMoveCostsCacheForSelfAndAdjacent();
}
void HasBlockFeatures::hue(const BlockFeatureType& blockFeatueType)
{
	assert(m_block.isSolid());
	m_features.emplace_back(blockFeatueType, m_block.getSolidMaterial(), true);
	m_block.clearMoveCostsCacheForSelfAndAdjacent();
}
