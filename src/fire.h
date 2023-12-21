#pragma once
#include "eventSchedule.h"
#include "temperature.h"
#include "eventSchedule.hpp"
#include "types.h"
#include <unordered_map>
#include <list>
#include <algorithm>
#include <string>

enum class FireStage{Smouldering, Burining, Flaming};

inline FireStage fireStageByName(std::string name)
{
	if(name == "Smouldering")
		return FireStage::Smouldering;
	if(name == "Burining")
		return FireStage::Burining;
	assert(name == "Flaming");
	return FireStage::Flaming;
}
inline std::string fireStageToString(FireStage fireStage)
{
	if(fireStage == FireStage::Smouldering)
		return "Smouldering";
	if(fireStage == FireStage::Burining)
		return "Burining";
	assert(fireStage == FireStage::Flaming);
	return "Flaming";
}


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

	FireEvent(Step delay, Fire& f, Step start);
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
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool operator==(const Fire& fire) const { return &fire == this; }
};
// To be used by Area.
class HasFires final
{
	std::unordered_map<Block*, std::unordered_map<const MaterialType*, Fire>> m_fires;
public:
	void ignite(Block& block, const MaterialType& materialType);
	void extinguish(Fire& fire);
	void load(Block& block, const MaterialType& materialType, bool hasPeaked, FireStage stage, Step start);
	// For testing.
	[[maybe_unused, nodiscard]] bool containsFireAt(Fire& fire, Block& block) const
	{ 
		assert(m_fires.contains(&block));
		return m_fires.at(&block).contains(&fire.m_materialType);
	}
};
