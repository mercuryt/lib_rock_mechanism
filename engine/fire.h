#pragma once
#include "temperature.h"
#include "eventSchedule.hpp"
#include "numericTypes/types.h"

#include <list>
#include <algorithm>
#include <string>

enum class FireStage{Smouldering, Burning, Flaming};
NLOHMANN_JSON_SERIALIZE_ENUM(FireStage, {
		{FireStage::Smouldering, "Smouldering"},
		{FireStage::Burning, "Burning"},
		{FireStage::Flaming, "Flaming"}
});
class Fire;
struct DeserializationMemo;
struct MaterialType;
/*
 * Progress through the stages of fire with scheduled events.
 */
class FireEvent final : public ScheduledEvent
{
public:
	Fire& m_fire;

	FireEvent(Area& area, const Step& delay, Fire& f, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class Fire final
{
public:
	TemperatureSource m_temperatureSource;
	HasScheduledEvent<FireEvent> m_event;
	Point3D m_location;
	MaterialTypeId m_solid;
	FireStage m_stage;
	bool m_hasPeaked;

	// Default arguments are used when creating a fire normally, custom values are for deserializing or dramatic use.
	Fire(Area& a, const Point3D& l, const MaterialTypeId& mt, bool hasPeaked = false, const FireStage stage = FireStage::Smouldering, const Step start = Step::null());
	[[nodiscard]] bool operator==(const Fire& fire) const { return &fire == this; }
};
class AreaHasFires final
{
	Area& m_area;
	// Outer map is hash because there are potentailly a large number of fires.
	// TODO: Swap to boost unordered map.
	std::unordered_map<Point3D, SmallMapStable<MaterialTypeId, Fire>, Point3D::Hash> m_fires;
public:
	AreaHasFires(Area& a) : m_area(a) { }
	void ignite(const Point3D& point, const MaterialTypeId& materialType);
	void extinguish(Fire& fire);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Fire& at(const Point3D& point, const MaterialTypeId& materialType);
	[[nodiscard]] bool contains(const Point3D& point, const MaterialTypeId& materialType);
	[[nodiscard]] Json toJson() const;
	// For testing.
	[[maybe_unused, nodiscard]] bool containsFireAt(Fire& fire, const Point3D& point) const
	{
		return m_fires.at(point).contains(fire.m_solid);
	}
};
