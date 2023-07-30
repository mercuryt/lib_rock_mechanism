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
// Name, hewn, blocks entrance, lockable, stand in, stand above
BlockFeatureType BlockFeatureType::floor = {"floor", false, false, false, true, false};
BlockFeatureType BlockFeatureType::door = {"door", false, false, true, false, false};
BlockFeatureType BlockFeatureType::hatch = {"hatch", false, false, true, true, false};
BlockFeatureType BlockFeatureType::stairs = {"stairs", true,  false, false, true, true};
BlockFeatureType BlockFeatureType::floodGate = {"floodGate", true, true, false, false, true};
BlockFeatureType BlockFeatureType::floorGrate = {"floorGrate", false, false, false, true, false};
BlockFeatureType BlockFeatureType::ramp = {"ramp", true, false, false, true, true};
BlockFeatureType BlockFeatureType::fortification = {"fortification", true, true, false, false, true};

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
	void setTemperature(uint32_t temperature); //TODO
	bool blocksEntrance() const;
	bool canStandAbove() const;
	bool canStandIn() const;
};
