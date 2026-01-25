#pragma once
#include "temperature.h"
#include "eventSchedule.hpp"
#include "numericTypes/types.h"

#include <list>
#include <algorithm>
#include <string>

enum class FireStage {Smouldering, Burning, Flaming};
NLOHMANN_JSON_SERIALIZE_ENUM(FireStage, {
		{FireStage::Smouldering, "Smouldering"},
		{FireStage::Burning, "Burning"},
		{FireStage::Flaming, "Flaming"}
});
class Fire;
struct DeserializationMemo;
struct MaterialType;
struct FireDelta
{
	Point3D location;
	MaterialTypeId materialType;
	[[nodiscard]] std::strong_ordering operator<=>(const FireDelta&) const = default;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FireDelta, location, materialType);
class Fire final
{
public:
	TemperatureSource m_temperatureSource;
	Point3D m_location;
	MaterialTypeId m_materialType;
	FireStage m_stage;
	bool m_hasPeaked;

	// Default arguments are used when creating a fire normally, custom values are for deserializing or dramatic use.
	Fire(Area& a, const Point3D& l, const MaterialTypeId& mt, bool hasPeaked = false, const FireStage stage = FireStage::Smouldering);
	void nextPhase(Area& area);
	[[nodiscard]] bool operator==(const Fire& fire) const;
	[[nodiscard]] FireDelta createDelta() const;
};
class AreaHasFires final
{
	// Outer map is hash because there are potentailly a large number of fires.
	// TODO: Swap to boost unordered map.
	std::unordered_map<Point3D, SmallMap<MaterialTypeId, Fire>, Point3D::Hash> m_fires;
	SmallMap<Step, SmallSet<FireDelta>> m_deltas;
public:
	void doStep(const Step& step, Area& area);
	void scheduleNextPhase(const Step& step, const Fire& fire);
	void ignite(Area& area, const Point3D& point, const MaterialTypeId& materialType);
	void extinguish(Area& area, Fire& fire);
	void load(Area& area, const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Fire& at(const Point3D& point, const MaterialTypeId& materialType);
	[[nodiscard]] bool contains(const Point3D& point, const MaterialTypeId& materialType);
	// Custom serialization is used here rather then NLOHMANN_DEFINE_TYPE because the heat delta is regenerated rather then serialized.
	[[nodiscard]] Json toJson() const;
	// For testing.
	[[nodiscard]] bool containsFireAt(Fire& fire, const Point3D& point) const;
	[[nodiscard]] bool containsDeltaFor(Fire& fire) const;
	[[nodiscard]] bool containsDeltas() const;
};
