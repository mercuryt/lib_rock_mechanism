#pragma once
#include "deserializationMemo.h"
#include "eventSchedule.h"
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
class Block;
struct MaterialType;
/*
 * Progress through the stages of fire with scheduled events.
 */
class FireEvent final : public ScheduledEventWithPercent
{
public:
	Fire& m_fire;

	FireEvent(Step delay, Fire& f, Step start = 0);
	void execute();
	void clearReferences();
};
class Fire final
{
public:
	Block& m_location;
	const MaterialType& m_materialType;
	HasScheduledEvent<FireEvent> m_event;
	FireStage m_stage;
	bool m_hasPeaked;
	TemperatureSource m_temperatureSource;

	// Default arguments are used when creating a fire normally, custom values are for deserializing or dramatic use.
	Fire(Block& l, const MaterialType& mt, bool hasPeaked = false, FireStage stage = FireStage::Smouldering, Step start = 0);
	[[nodiscard]] bool operator==(const Fire& fire) const { return &fire == this; }
};
class AreaHasFires final
{
	std::unordered_map<Block*, std::unordered_map<const MaterialType*, Fire>> m_fires;
public:
	void ignite(Block& block, const MaterialType& materialType);
	void extinguish(Fire& fire);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	// For testing.
	[[maybe_unused, nodiscard]] bool containsFireAt(Fire& fire, Block& block) const
	{ 
		assert(m_fires.contains(&block));
		return m_fires.at(&block).contains(&fire.m_materialType);
	}
};
