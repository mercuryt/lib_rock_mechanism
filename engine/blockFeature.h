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
	std::vector<BlockFeature> m_features;
	Block& m_block;
	[[nodiscard]] BlockFeature* at(const BlockFeatureType& blockFeatueType);
public:
	HasBlockFeatures(Block& b) : m_block(b) { }

	// Can return nullptr.
	void remove(const BlockFeatureType& blockFeatueType);
	void removeAll();
	void construct(const BlockFeatureType& blockFeatueType, const MaterialType& materialType);
	void hew(const BlockFeatureType& blockFeatueType);
	void setTemperature(Temperature temperature);
	void lock(const BlockFeatureType& blockFeatueType);
	void unlock(const BlockFeatureType& blockFeatueType);
	void close(const BlockFeatureType& blockFeatueType);
	void open(const BlockFeatureType& blockFeatueType);
	[[nodiscard]] const BlockFeature* atConst(const BlockFeatureType& blockFeatueType) const;
	[[nodiscard]] const std::vector<BlockFeature>& get() const { return m_features; }
	[[nodiscard]] bool empty() const { return m_features.empty(); }
	[[nodiscard]] bool blocksEntrance() const;
	[[nodiscard]] bool canStandAbove() const;
	[[nodiscard]] bool canStandIn() const;
	[[nodiscard]] bool isSupport() const;
	[[nodiscard]] bool canEnterFromBelow() const;
	[[nodiscard]] bool canEnterFromAbove(const Block& from) const;
	[[nodiscard]] const MaterialType* getMaterialType() const;
	[[nodiscard]] bool contains(const BlockFeatureType& blockFeatureType) const;
};
