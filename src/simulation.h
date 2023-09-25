#pragma once
#include "../lib/BS_thread_pool_light.hpp"
#include "area.h"
#include "types.h"
#include "datetime.h"
#include "eventSchedule.hpp"
#include "config.h"
#include "threadedTask.h"
#include "actor.h"
#include "item.h"
#include <list>

class HourlyEvent;

class Simulation
{
	std::list<Area> m_areas;
public:
	BS::thread_pool_light m_pool;
	Step m_step;
	DateTime m_now;
	uint32_t m_nextActorId;
	std::list<Actor> m_actors;
	uint32_t m_nextItemId;
	std::list<Item> m_items;
	EventSchedule m_eventSchedule;
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	ThreadedTaskEngine m_threadedTaskEngine;
	Simulation(DateTime n = {12, 150, 1200}, Step s = 1);
	void doStep();
	void incrementHour();
	//TODO: latitude, longitude, altitude.
	inline Area& createArea(uint32_t x, uint32_t y, uint32_t z) { return m_areas.emplace_back(*this, x, y, z); }
	Actor& createActor(const AnimalSpecies& species, Block& block, Percent percentGrown = 100);
	// Non generic, unnamed, no id.
	Item& createItem(const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, CraftJob* cj = nullptr);
	// Named, no id.
	Item& createItem(const ItemType& itemType, const MaterialType& materialType, std::string name, uint32_t quality, Percent percentWear, CraftJob* cj = nullptr);
	// Generic, no id.
	Item& createItem(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj = nullptr);
	Item& createItem(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj = nullptr);
	// Named, has id.
	Item& createItem(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, std::string name, uint32_t quality, Percent percentWear, CraftJob* cj = nullptr);
	~Simulation();
	// For testing.
	[[maybe_unused]] void setDateTime(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Actor& actor, Block& destination);
};

class HourlyEvent final : public ScheduledEventWithPercent
{
	Simulation& m_simulation;
public:
	HourlyEvent(Simulation& s) : ScheduledEventWithPercent(s, Config::stepsPerHour), m_simulation(s) { }
	inline void execute(){ m_simulation.incrementHour(); }
	inline void clearReferences(){ m_simulation.m_hourlyEvent.clearPointer(); }
};
