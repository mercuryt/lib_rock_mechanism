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
	static BlockFeatureType flap;
	static BlockFeatureType hatch;
	static BlockFeatureType stairs;
	static BlockFeatureType floodGate;
	static BlockFeatureType floorGrate;
	static BlockFeatureType ramp;
	static BlockFeatureType fortification;
	static void load();
	static std::vector<BlockFeatureType*> getAll() { return std::vector{&floor, &door, &flap, &hatch, &stairs, &floodGate, &floorGrate, &ramp, &fortification};  }
	static const BlockFeatureType& byName(std::string name)
	{
		if(name == "floor")
			return floor;
		if(name == "door")
			return door;
		if(name == "flap")
			return flap;
		if(name == "hatch")
			return hatch;
		if(name == "stairs")
			return stairs;
		if(name == "floodGate")
			return floodGate;
		if(name == "floorGrate")
			return floorGrate;
		if(name == "ramp")
			return ramp;
		assert(name == "fortification");
		return fortification;
	}
};
inline void to_json(Json& data, const BlockFeatureType* const & blockFeatueType){ data = blockFeatueType->name; }
struct BlockFeature
{
	// Use pointers rather then references so we can store in vector and be able to erase at arbirtary index.
	const BlockFeatureType* blockFeatureType;
	const MaterialType* materialType;
	// TODO: Replace hewn with ItemType* to differentiate between walls made of carved blocks from those made from uncut stone or between wood planks and logs.
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
	void setTemperature(Temperature temperature);
	bool blocksEntrance() const;
	bool canStandAbove() const;
	bool canStandIn() const;
	bool isSupport() const;
	bool canEnterFromBelow() const;
	bool canEnterFromAbove(const Block& from) const;
};
