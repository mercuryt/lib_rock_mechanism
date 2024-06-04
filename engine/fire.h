#pragma once
#include "temperature.h"
#include "eventSchedule.hpp"
#include "types.h"

#include <unordered_map>
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
	Area& m_area;
public:
	Fire& m_fire;

	FireEvent(Area& area, Step delay, Fire& f, Step start = 0);
	void execute();
	void clearReferences();
};
class Fire final
{
	Area& m_area;
public:
	TemperatureSource m_temperatureSource;
	HasScheduledEvent<FireEvent> m_event;
	BlockIndex m_location;
	const MaterialType& m_materialType;
	FireStage m_stage;
	bool m_hasPeaked;

	// Default arguments are used when creating a fire normally, custom values are for deserializing or dramatic use.
	Fire(Area& a, BlockIndex l, const MaterialType& mt, bool hasPeaked = false, FireStage stage = FireStage::Smouldering, Step start = 0);
	[[nodiscard]] bool operator==(const Fire& fire) const { return &fire == this; }
};
class AreaHasFires final
{
	Area& m_area;
	std::unordered_map<BlockIndex, std::unordered_map<const MaterialType*, Fire>> m_fires;
public:
	AreaHasFires(Area& a) : m_area(a) { }
	void ignite(BlockIndex block, const MaterialType& materialType);
	void extinguish(Fire& fire);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Fire& at(BlockIndex block, const MaterialType& materialType);
	[[nodiscard]] bool contains(BlockIndex block, const MaterialType& materialType);
	[[nodiscard]] Json toJson() const;
	// For testing.
	[[maybe_unused, nodiscard]] bool containsFireAt(Fire& fire, BlockIndex block) const
	{ 
		return m_fires.at(block).contains(&fire.m_materialType);
	}
};
