#pragma once

#include "materialType.h"

class Block;

struct BlockFeatureType
{
	const std::string name;
	const bool canBeHewn;
	const bool blocksEntrance;
	const bool lockable;
	const bool canStandIn;
	const bool canStandAbove;
	const bool isSupportAgainstCaveIn;
	bool operator==(const BlockFeatureType& x) const { return this == &x; }
	static BlockFeatureType floor;
	static BlockFeatureType door;
	static BlockFeatureType hatch;
	static BlockFeatureType stairs;
	static BlockFeatureType floodGate;
	static BlockFeatureType floorGrate;
	static BlockFeatureType ramp;
	static BlockFeatureType fortification;
	static void load();
};

struct BlockFeature
{
	// Use pointers rather then references so we can store in vector and be able to erase at arbirtary index.
	const BlockFeatureType* blockFeatureType;
	const MaterialType* materialType;
	bool hewn;
	bool closed;
	bool locked;
	BlockFeature(const BlockFeatureType& bft, const MaterialType& mt, bool h) : blockFeatureType(&bft), materialType(&mt), hewn(h), closed(true), locked(false) {}
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
	const BlockFeature* at(const BlockFeatureType& blockFeatueType) const;
	const std::vector<BlockFeature>& get() const { return m_features; }
	bool empty() const { return m_features.empty(); }
	void remove(const BlockFeatureType& blockFeatueType);
	void construct(const BlockFeatureType& blockFeatueType, const MaterialType& materialType);
	void hew(const BlockFeatureType& blockFeatueType);
	void setTemperature(uint32_t temperature);
	bool blocksEntrance() const;
	bool canStandAbove() const;
	bool canStandIn() const;
	bool isSupport() const;
	bool canEnterFromBelow() const;
	bool canEnterFromAbove(const Block& from) const;
};
