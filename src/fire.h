#pragma once
#include "eventSchedule.h"
#include "temperature.h"
#include "eventSchedule.hpp"
#include <unordered_map>
#include <list>
#include <algorithm>

enum class FireStage{Smouldering, Burining, Flaming};

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

	FireEvent(uint32_t delay, Fire& f);
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

	Fire(Block& l, const MaterialType& mt);
	[[nodiscard]] bool operator==(const Fire& fire) const { return &fire == this; }
};
// To be used by Area.
class HasFires final
{
	std::unordered_map<Block*, std::unordered_map<const MaterialType*, Fire>> m_fires;
public:
	void ignite(Block& block, const MaterialType& materialType);
	void extinguish(Fire& fire);
	// For testing.
	[[maybe_unused, nodiscard]] bool containsFireAt(Fire& fire, Block& block) const
	{ 
		assert(m_fires.contains(&block));
		return m_fires.at(&block).contains(&fire.m_materialType);
	}
};
