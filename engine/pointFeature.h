#pragma once

#include "definitions/materialType.h"
#include "numericTypes/types.h"
#include "dataStructures/bitset.h"

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
	const bool opaque;
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
	// Floor and hatch are not marked as opaque, they are handled seperately.
	{"floor", false, false, false, true, false, false, true, false, Force::create(0), 1},
	{"floorGrate", false, false, false, true, false, false, true, false, Force::create(0), 1},
	{"fortification", true, true, false, false, true, true, true, false, Force::create(0), 1},
	{"hatch", false, false, true, true, false, false, true, false, Force::create(0), 1},
	{"keel", false, true, false, false, true, true, true, true, Force::create(0), 1},
	{"ramp", true, false, false, true, true, false, true, false, Force::create(0), 1},
	{"stairs", true, false, false, true, true, false, true, false, Force::create(0), 1},
}};

struct PointFeature
{
	struct Primitive
	{
		MaterialTypeId::Primitive materialType;
		PointFeatureTypeId pointFeatureType;
		uint8_t hewnAndClosedAndLocked;
		[[nodiscard]] constexpr std::strong_ordering operator<=>(const Primitive& other) const { return pointFeatureType <=> other.pointFeatureType; }
		[[nodiscard]] constexpr bool operator==(const Primitive& other) const = default;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Primitive, materialType, pointFeatureType, hewnAndClosedAndLocked);
	};
	MaterialTypeId materialType;
	PointFeatureTypeId pointFeatureType = PointFeatureTypeId::Null;
	// TODO: Replace hewn with ItemTypeId to differentiate between walls made of carved space from those made from uncut stone or between wood planks and logs?
	// Initalize hewn as false, closed as true, and locked as false.
	BitSet<uint8_t, 3> hewnAndClosedAndLocked;
	void setClosed(bool setTo) { hewnAndClosedAndLocked.set(1, setTo); }
	void setLocked(bool setTo) { hewnAndClosedAndLocked.set(2, setTo); }
	void clear();
	[[nodiscard]] constexpr bool isHewn() const { return hewnAndClosedAndLocked.test(0); }
	[[nodiscard]] constexpr bool isClosed() const { return hewnAndClosedAndLocked.test(1); }
	[[nodiscard]] constexpr bool isLocked() const { return hewnAndClosedAndLocked.test(2); }
	[[nodiscard]] constexpr bool exists() const { return materialType.exists(); }
	[[nodiscard]] constexpr Primitive get() const { return {materialType.get(), pointFeatureType, hewnAndClosedAndLocked.data}; }
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const PointFeature& other) const { return pointFeatureType <=> other.pointFeatureType; }
	[[nodiscard]] constexpr bool operator==(const PointFeature& other) const = default;
	[[nodiscard]] std::string toString() const;
	[[nodiscard]] constexpr static PointFeature null() { return {}; }
	[[nodiscard]] constexpr static Primitive nullPrimitive() { return {.materialType=MaterialTypeId::nullPrimitive(), .pointFeatureType=PointFeatureTypeId::Null, .hewnAndClosedAndLocked=0}; }
	[[nodiscard]] constexpr static PointFeature create(const Primitive& primitive) { return {MaterialTypeId::create(primitive.materialType), primitive.pointFeatureType, BitSet<uint8_t, 3>::create(primitive.hewnAndClosedAndLocked)}; }
	[[nodiscard]] constexpr static PointFeature create(const MaterialTypeId& materialType, const PointFeatureTypeId& pointFeatureType, bool hewn = false, bool closed = true, bool locked = false)
	{
		BitSet<uint8_t, 3> hewnAndClosedAndLocked;
		hewnAndClosedAndLocked.set(0, hewn);
		hewnAndClosedAndLocked.set(1, closed);
		hewnAndClosedAndLocked.set(2, locked);
		PointFeature output{materialType, pointFeatureType, hewnAndClosedAndLocked};
		return output;
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(PointFeature, materialType, pointFeatureType, hewnAndClosedAndLocked);
private:
	void setHewn(bool setTo) { hewnAndClosedAndLocked.set(0, setTo); }
};

