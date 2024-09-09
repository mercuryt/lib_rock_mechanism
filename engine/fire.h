#pragma once
#include "temperature.h"
#include "eventSchedule.hpp"
#include "types.h"

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

	FireEvent(Area& area, Step delay, Fire& f, Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class Fire final
{
public:
	TemperatureSource m_temperatureSource;
	HasScheduledEvent<FireEvent> m_event;
	BlockIndex m_location;
	MaterialTypeId m_materialType;
	FireStage m_stage;
	bool m_hasPeaked;

	// Default arguments are used when creating a fire normally, custom values are for deserializing or dramatic use.
	Fire(Area& a, BlockIndex l, MaterialTypeId mt, bool hasPeaked = false, FireStage stage = FireStage::Smouldering, Step start = Step::null());
	[[nodiscard]] bool operator==(const Fire& fire) const { return &fire == this; }
};
class AreaHasFires final
{
	Area& m_area;
	// Outer map is hash because there are potentailly a large number of fires.
	// Inner map is hash because Fire is immobile, due to containing a scheduled Event.
	// TODO: stable small map.
	std::unordered_map<BlockIndex, SmallMapStable<MaterialTypeId, Fire>, BlockIndex::Hash> m_fires;
public:
	AreaHasFires(Area& a) : m_area(a) { }
	void ignite(BlockIndex block, MaterialTypeId materialType);
	void extinguish(Fire& fire);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Fire& at(BlockIndex block, MaterialTypeId materialType);
	[[nodiscard]] bool contains(BlockIndex block, MaterialTypeId materialType);
	[[nodiscard]] Json toJson() const;
	// For testing.
	[[maybe_unused, nodiscard]] bool containsFireAt(Fire& fire, BlockIndex block) const
	{ 
		return m_fires.at(block).contains(fire.m_materialType);
	}
};
