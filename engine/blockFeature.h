#pragma once

#include "materialType.h"
#include "types.h"

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
inline void to_json(Json& data, const BlockFeatureType* const& blockFeatureType){ data = blockFeatureType->name; }
inline void from_json(const Json& data, const BlockFeatureType*& blockFeatureType){ blockFeatureType = &BlockFeatureType::byName(data.get<std::string>()); }
struct BlockFeature
{
	// Use pointers rather then references so we can store in vector and be able to erase at arbirtary index.
	const BlockFeatureType* blockFeatureType;
	MaterialTypeId materialType;
	// TODO: Replace hewn with ItemType* to differentiate between walls made of carved blocks from those made from uncut stone or between wood planks and logs.
	bool hewn = false;
	bool closed = true;
	bool locked = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BlockFeature, blockFeatureType, materialType, hewn, closed, locked);
