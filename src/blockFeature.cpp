#include "blockFeature.h"
void BlockFeatureType::load()
{
	BlockFeatureType::floor("floor", false);
	BlockFeatureType::door("door", false);
	BlockFeatureType::hatch("hatch", false);
	BlockFeatureType::floodGate("floodGate", false);
	BlockFeatureType::floorGrate("floorGrate", false);
	BlockFeatureType::fortification("fortification", false);
	BlockFeatureType::stairs("stairs", false);
	BlockFeatureType::ramp("ramp", false);
}
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
const BlockFeature* HasBlockFeatures::at(const BlockFeatureType& blockFeatueType) const
{
	return const_cast<const HasBlockFeatures&>(*this).at(blockFeatueType);
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
void HasBlockFeatures::hew(const BlockFeatureType& blockFeatueType)
{
	assert(m_block.isSolid());
	m_features.emplace_back(blockFeatueType, m_block.getSolidMaterial(), true);
	m_block.clearMoveCostsCacheForSelfAndAdjacent();
}
bool HasBlockFeatures::blocksEntrance() const
{
	for(const BlockFeature& blockFeature : m_block.m_features.get())
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::fortification || blockFeature.blockFeatureType == &BlockFeatureType::floodGate ||
				(blockFeature.blockFeatureType == &BlockFeatureType::door && blockFeature.locked)
		  )
			return false;
	}
	return true;
}
