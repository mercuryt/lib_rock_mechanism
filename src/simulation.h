#pragma once
#include "../lib/BS_thread_pool_light.hpp"
#include "types.h"
#include "area.h"
#include "project.h"
#include "types.h"
#include "config.h"
#include "datetime.h"
#include "eventSchedule.hpp"
#include "config.h"
#include "threadedTask.h"
#include "actor.h"
#include "item.h"
#include "random.h"
#include <list>
#include <unordered_map>

class HourlyEvent;

class Simulation final
{
	AreaId m_nextAreaId;
	std::list<Area> m_areas;
	std::unordered_map<AreaId, Area*> m_areasById;
public:
	BS::thread_pool_light m_pool;
	Step m_step;
	DateTime m_now;
	ActorId m_nextActorId;
	std::unordered_map<ActorId, Actor> m_actors;
	ItemId m_nextItemId;
	std::unordered_map<ItemId, Item> m_items;
	EventSchedule m_eventSchedule;
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	ThreadedTaskEngine m_threadedTaskEngine;
	Random m_random;
	SimulationHasFactions m_hasFactions;
	Simulation(DateTime n = {12, 150, 1200}, Step s = 1);
	Simulation(const Json& data);
	Json toJson() const;
	void doStep();
	void incrementHour();
	//TODO: latitude, longitude, altitude.
	Area& createArea(uint32_t x, uint32_t y, uint32_t z);
	Area& loadArea(AreaId id, uint32_t x, uint32_t y, uint32_t z);
	Actor& createActor(const AnimalSpecies& species, Block& block, Percent percentGrown = 100);
	// Non generic, no id.
	Item& createItemNongeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, CraftJob* cj = nullptr);
	// Generic, no id.
	Item& createItemGeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj = nullptr);
	// Non generic with id.
	Item& loadItemNongeneric(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, std::wstring name, CraftJob* cj = nullptr);
	// Generic, with id.
	Item& loadItemGeneric(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj = nullptr);
	void destroyItem(Item& item);
	void destroyArea(Area& area);
	void loadAreaFromJson(const Json& data);
	Item& loadItemFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	Actor& loadActorFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	Block& getBlockForJsonQuery(const Json& data);
	Actor& getActorById(ActorId id);
	Item& getItemById(ItemId id);
	~Simulation();
	// For testing.
	[[maybe_unused]] void setDateTime(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAtDestination(Actor& actor, Block& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAt(Actor& actor, Block& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Actor& actor, Block& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentTo(Actor& actor, Block& block);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToHasShape(Actor& actor, HasShape& other);
	[[maybe_unused]] void fastForwardUntillActorHasNoDestination(Actor& actor);
	[[maybe_unused]] void fastForwardUntillPredicate(std::function<bool()> predicate, uint32_t minutes = 10);
};

class HourlyEvent final : public ScheduledEventWithPercent
{
	Simulation& m_simulation;
public:
	HourlyEvent(Simulation& s, const Step start = 0) : ScheduledEventWithPercent(s, Config::stepsPerHour, start), m_simulation(s) { }
	inline void execute(){ m_simulation.incrementHour(); }
	inline void clearReferences(){ m_simulation.m_hourlyEvent.clearPointer(); }
};
