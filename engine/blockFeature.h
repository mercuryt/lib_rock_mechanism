#pragma once

#include "definitions/materialType.h"
#include "numericTypes/types.h"


struct BlockFeature
{
	BlockFeatureTypeId blockFeatureTypeId;
	MaterialTypeId materialType;
	// TODO: Replace hewn with ItemType* to differentiate between walls made of carved blocks from those made from uncut stone or between wood planks and logs.
	// TODO: collapse these into a bitset.
	bool hewn = false;
	bool closed = true;
	bool locked = false;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlockFeature, blockFeatureTypeId, materialType, hewn, closed, locked);
};

struct BlockFeatureType
{
	const std::string name;
	const bool canBeHewn;
	const bool blocksEntrance;
	const bool lockable;
	const bool canStandIn;
	const bool canStandAbove;
	const bool isSupportAgainstCaveIn;
	const bool blocksMultiTileShapesIfNotAtZeroZOffset;
	const bool blockIsOpaque;
	const Force motiveForce;
	const uint value;
	bool operator==(const BlockFeatureType& x) const { return this == &x; }
	static const BlockFeatureType& byName(const std::string& name);
	static const BlockFeatureType& byId(const BlockFeatureTypeId& id);
};
// These must appear in the same order as in the enum.
static std::array<BlockFeatureType, 10> blockFeatureTypeData = {{
	{"door", false, false, true, false, false, false, true, true, Force::create(0), 1},
	{"flap", false, false, false, false, false, false, true, true, Force::create(0), 1},
	{"floodGate", true, true, false, false, true, false, true, false, Force::create(0), 1},
	{"floor", false, false, false, true, false, false, true, false, Force::create(0), 1},
	{"floorGrate", false, false, false, true, false, false, true, false, Force::create(0), 1},
	{"fortification", true, true, false, false, true, true, true, false, Force::create(0), 1},
	{"hatch", false, false, true, true, false, false, true, false, Force::create(0), 1},
	{"keel", false, true, false, false, true, true, true, true, Force::create(0), 1},
	{"ramp", true, false, false, true, true, false, true, false, Force::create(0), 1},
	{"stairs", true, false, false, true, true, false, true, false, Force::create(0), 1},
}};
class BlockFeatureSet
{
	std::vector<BlockFeature> m_data;
public:
	void insert(const BlockFeatureTypeId& typeId, const MaterialTypeId& materialType, bool hewn, bool closed, bool locked);
	void remove(const BlockFeatureTypeId& typeId);
	void clear() { m_data.clear(); }
	[[nodiscard]] bool contains(const BlockFeatureTypeId& typeId) const;
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool canStandIn() const;
	[[nodiscard]] bool canStandAbove() const;
	[[nodiscard]] bool blocksEntrance() const;
	[[nodiscard]] bool isOpaque() const;
	[[nodiscard]] bool floorIsOpaque() const;
	[[nodiscard]] const BlockFeature& front() const { return m_data.front(); }
	[[nodiscard]] const BlockFeature& get(const BlockFeatureTypeId& typeId) const { return const_cast<BlockFeatureSet&>(*this).get(typeId); }
	[[nodiscard]] BlockFeature& get(const BlockFeatureTypeId& typeId);
	[[nodiscard]] BlockFeature* maybeGet(const BlockFeatureTypeId& typeId);
	[[nodiscard]] const auto begin() const { return m_data.begin(); }
	[[nodiscard]] const auto end() const { return m_data.end(); }
	[[nodiscard]] auto begin() { return m_data.begin(); }
	[[nodiscard]] auto end() { return m_data.end(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlockFeatureSet, m_data);
};
