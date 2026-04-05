#pragma once
#include "eventSchedule.hpp"
#include "numericTypes/types.h"
#include "dataStructures/smallMap.h"
#include "geometry/point3D.h"

#include <list>
#include <algorithm>
#include <string>

enum class FireStage {Smouldering, Burning, Flaming};
NLOHMANN_JSON_SERIALIZE_ENUM(FireStage, {
		{FireStage::Smouldering, "Smouldering"},
		{FireStage::Burning, "Burning"},
		{FireStage::Flaming, "Flaming"}
});
struct FireDelta
{
	Point3D location;
	MaterialTypeId materialType;
	[[nodiscard]] std::strong_ordering operator<=>(const FireDelta&) const = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FireDelta, location, materialType);
};
struct Fire final
{
	TemperatureSourceId m_temperatureSource;
	Point3D m_location;
	MaterialTypeId m_materialType;
	FireStage m_stage;
	bool m_hasPeaked;
	void nextPhase(Area& area);
	[[nodiscard]] bool operator==(const Fire& fire) const;
	[[nodiscard]] FireDelta createDelta() const;
	[[nodiscard]] TemperatureDelta getTemperatureDelta() const;
	// Default arguments are used when creating a fire normally, custom values are for dramatic use.
	[[nodiscard]] static Fire create(Area& area, Point3D location, MaterialTypeId materialType, bool hasPeaked = false, FireStage stage = FireStage::Smouldering);
};
void to_json(Json& j, const Fire& fire);
void from_json(const Json& j, Fire& fire);
struct AreaHasFires final
{
	// Outer map is hash because there are potentailly a large number of fires.
	// TODO: Swap to boost unordered map.
	std::unordered_map<Point3D, SmallMap<MaterialTypeId, Fire>, Point3D::Hash> m_fires;
	SmallMap<Step, SmallSet<FireDelta>> m_deltas;
	void doStep(const Step step, Area& area);
	void scheduleNextPhase(const Step step, const Fire& fire);
	void ignite(Area& area, const Point3D point, const MaterialTypeId materialType);
	void extinguish(Area& area, Fire& fire);
	[[nodiscard]] Fire& at(const Point3D point, const MaterialTypeId materialType);
	[[nodiscard]] bool contains(const Point3D point, const MaterialTypeId materialType);
	// For testing.
	[[nodiscard]] bool containsFireAt(Fire& fire, const Point3D point) const;
	[[nodiscard]] bool containsDeltaFor(Fire& fire) const;
	[[nodiscard]] bool containsDeltas() const;
};
void to_json(Json& j, const AreaHasFires& a);
void from_json(const Json& j, AreaHasFires& a);
