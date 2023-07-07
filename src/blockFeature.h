#pragma once

#include "materialType.h"

class Block;

struct BlockFeatureType
{
	const std::string name;
	const bool canBeHewn;
	bool operator==(const BlockFeatureType& x) const { return this == &x; }
};
struct BlockFeature
{
	const BlockFeatureType& blockFeatureType;
	const MaterialType& materialType;
	bool hewn;
	bool closed;
	bool locked;
	BlockFeature(const BlockFeatureType& bft, const MaterialType& mt, bool h) : blockFeatureType(bft), materialType(mt), hewn(h), closed(true), locked(false) {}
};
class HasBlockFeatures
{
	Block& m_block;
	std::vector<BlockFeature> m_features;
public:
	HasBlockFeatures(Block& b) : m_block(b) { }
	bool contains(const BlockFeatureType& blockFeatureType) const;
	// Can return nullptr.
	BlockFeature* at(const BlockFeatureType& blockFeatueType);
	void remove(const BlockFeatureType& blockFeatueType);
	void construct(const BlockFeatureType& blockFeatueType, const MaterialType& materialType);
	void hue(const BlockFeatureType& blockFeatueType);
};
