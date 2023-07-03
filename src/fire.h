#pragma once
#include "eventSchedule.h"
#include "temperatureSource.h"
#include <memory>

constexpr int32_t heatFractionForSmoulder = 10;
constexpr int32_t heatFractionForBurn = 3;
constexpr int32_t rampDownPhaseDurationFraction = 2;

enum class FireStage{Smouldering, Burining, Flaming};

class Fire;
/*
 * Progress through the stages of fire with scheduled events. Fire stores a pointer to the current event so it can be canceled if it is deleted ( extinguished ).
 */
class FireEvent : public ScheduledEvent
{
public:
	Fire& m_fire;

	FireEvent(uint32_t s, Fire& f) : ScheduledEvent(s), m_fire(f) {}
	void execute();
	~FireEvent();
};
class Fire
{
public:
	Block* m_location;
	const MaterialType& m_materialType;
	HasScheduledEvent<FireEvent> m_event;
	FireStage m_stage;
	bool m_hasPeaked;
	TemperatureSource m_temperatureSource;

	Fire(Block& l, const MaterialType& mt);
};
// To be used by Area.
class HasFires
{
	std::unordered_map<Block*, std::list<Fire>> m_fires;
public:
	void add(Block& block, const MaterialType& materialType);
	void remove(Block& block, const MaterialType& materialType);
};
