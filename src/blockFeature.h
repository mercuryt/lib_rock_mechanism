#pragma once

struct BlockFeatureType
{
	const std::string name;
	const bool canBeHewn;
};
template<class BlockFeatureType>
struct BlockFeature
{
	const BlockFeatureType& blockFeatureType;
	const MaterialType& materialType;
	bool hewn;
	bool closed;
	bool locked;
	BlockFeature(const BlockFeatureType& bft, const MaterialType& mt, bool h) : blockFeatureType(&bft), materialType(&mt), hewn(h), closed(true), locked(false) {}
};
template<class Block, class BlockFeature, class BlockFeatureType>
class HasBlockFeatures
{
	Block& m_block;
	std::vector<BlockFeature> m_features;
public:
	HasBlockFeatures(Block& b) : m_block(b) { }
	bool contains(const BlockFeatureType& blockFeatureType) const
	{
		return m_features.end() != std::ranges::find(m_features, blockFeatueType, &BlockFeature::blockFeatureType);
	}
	// Can return nullptr.
	BlockFeature* at(const BlockFeatureType& blockFeatueType)
	{
		return &*std::ranges::find(m_features, blockFeatueType, &BlockFeature::blockFeatureType)
	}
	void remove(const BlockFeatureType& blockFeatueType)
	{
		std::erase_if(m_features, [&](BlockFeature& bf) { return bf.blockFeatureType == blockFeatueType; });
		m_block.m_area->expireRouteCache();
		m_block.clearMoveCostsCacheForSelfAndAdjacent();
	}
	void construct(const BlockFeatureType& blockFeatueType, const MaterialType& materialType)
	{
		m_features.emplace_back(blockFeatueType, materialType, false);
		m_block.m_area->expireRouteCache();
		m_block.clearMoveCostsCacheForSelfAndAdjacent();
	}
	void hue(const BlockFeatureType& blockFeatueType)
	{
		assert(m_block.isSolid());
		m_features.emplace_back(blockFeatueType, m_block.getSolidMaterial(), true);
		m_block.m_area->expireRouteCache();
		m_block.clearMoveCostsCacheForSelfAndAdjacent();
	}
};
