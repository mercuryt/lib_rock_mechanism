#pragma once
#include "eventSchedule.h"
#include "temperature.h"
#include "eventSchedule.hpp"

enum class FireStage{Smouldering, Burining, Flaming};

class Fire;
class Block;
struct MaterialType;
/*
 * Progress through the stages of fire with scheduled events.
 */
class FireEvent : public ScheduledEventWithPercent
{
public:
	Fire& m_fire;

	FireEvent(uint32_t s, Fire& f) : ScheduledEventWithPercent(s), m_fire(f) {}
	void execute();
	~FireEvent();
};
class Fire
{
public:
	Block& m_location;
	const MaterialType& m_materialType;
	HasScheduledEvent<FireEvent> m_event;
	FireStage m_stage;
	bool m_hasPeaked;
	TemperatureSource m_temperatureSource;

	Fire(Block& l, const MaterialType& mt);
	bool operator==(const Fire& fire) const { return &fire == this; }
};
// To be used by Area.
class HasFires
{
	std::unordered_map<Block*, std::list<Fire>> m_fires;
public:
	void ignite(Block& block, const MaterialType& materialType);
	void extinguish(Fire& fire);
};
