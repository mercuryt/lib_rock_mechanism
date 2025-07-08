#pragma once

#include "definitions/materialType.h"
#include "numericTypes/types.h"


struct PointFeature
{
	PointFeatureTypeId pointFeatureTypeId;
	MaterialTypeId materialType;
	// TODO: Replace hewn with ItemType* to differentiate between walls made of carved space from those made from uncut stone or between wood planks and logs.
	// TODO: collapse these into a bitset.
	bool hewn = false;
	bool closed = true;
	bool locked = false;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointFeature, pointFeatureTypeId, materialType, hewn, closed, locked);
	[[nodiscard]] bool operator==(const PointFeature& other) const = default;
};

struct PointFeatureType
{
	const std::string name;
	const bool canBeHewn;
	const bool blocksEntrance;
	const bool lockable;
	const bool canStandIn;
	const bool canStandAbove;
	const bool isSupportAgainstCaveIn;
	const bool blocksMultiTileShapesIfNotAtZeroZOffset;
	const bool pointIsOpaque;
	const Force motiveForce;
	const uint value;
	bool operator==(const PointFeatureType& x) const { return this == &x; }
	static const PointFeatureType& byName(const std::string& name);
	static const PointFeatureType& byId(const PointFeatureTypeId& id);
};
// These must appear in the same order as in the enum.
static std::array<PointFeatureType, 10> pointFeatureTypeData = {{
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
class PointFeatureSet
{
	std::vector<PointFeature> m_data;
public:
	void insert(const PointFeatureTypeId& typeId, const MaterialTypeId& materialType, bool hewn, bool closed, bool locked);
	void remove(const PointFeatureTypeId& typeId);
	void clear() { m_data.clear(); }
	[[nodiscard]] bool contains(const PointFeatureTypeId& typeId) const;
	[[nodiscard]] bool empty() const { return m_data.empty(); }
	[[nodiscard]] bool canStandIn() const;
	[[nodiscard]] bool canStandAbove() const;
	[[nodiscard]] bool blocksEntrance() const;
	[[nodiscard]] bool isOpaque() const;
	[[nodiscard]] bool floorIsOpaque() const;
	[[nodiscard]] const PointFeature& front() const { return m_data.front(); }
	[[nodiscard]] const PointFeature& get(const PointFeatureTypeId& typeId) const { return const_cast<PointFeatureSet&>(*this).get(typeId); }
	[[nodiscard]] const PointFeature* maybeGet(const PointFeatureTypeId& typeId) const { return const_cast<PointFeatureSet&>(*this).maybeGet(typeId); }
	[[nodiscard]] PointFeature& get(const PointFeatureTypeId& typeId);
	[[nodiscard]] PointFeature* maybeGet(const PointFeatureTypeId& typeId);
	[[nodiscard]] const auto begin() const { return m_data.begin(); }
	[[nodiscard]] const auto end() const { return m_data.end(); }
	[[nodiscard]] auto begin() { return m_data.begin(); }
	[[nodiscard]] auto end() { return m_data.end(); }
	[[nodiscard]] auto size() const { return m_data.size(); }
	[[nodiscard]] bool operator==(const PointFeatureSet& other) const = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointFeatureSet, m_data);
};
