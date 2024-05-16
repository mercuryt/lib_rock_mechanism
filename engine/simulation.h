#pragma once
#include "../lib/BS_thread_pool_light.hpp"
#include "deserializationMemo.h"
#include "dialogueBox.h"
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
#include "random.h"
#include "input.h"
#include "uniform.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <mutex>

//class World;
class HourlyEvent;
class DramaEngine;
class SimulationHasItems;

class Simulation final
{
	AreaId m_nextAreaId;
	std::future<void> m_stepFuture;
	DeserializationMemo m_deserializationMemo;
public:
	std::unordered_map<AreaId, Area*> m_areasById;
	BS::thread_pool_light m_pool;
	std::vector<std::future<void>> m_taskFutures;
	std::mutex m_uiReadMutex;
	std::wstring m_name;
	std::filesystem::path m_path;
	Step m_step;
	ActorId m_nextActorId;
	//std::unique_ptr<World> m_world;
	std::list<Area> m_areas;
	std::unordered_map<ActorId, Actor> m_actors;
	EventSchedule m_eventSchedule;
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	ThreadedTaskEngine m_threadedTaskEngine;
	Random m_random;
	InputQueue m_inputQueue;
	SimulationHasUniforms m_hasUniforms;
	SimulationHasShapes m_shapes;
	SimulationHasFactions m_hasFactions;
	DialogueBoxQueue m_hasDialogues;
	// Dependency injectien.
	std::unique_ptr<DramaEngine> m_dramaEngine;
	std::unique_ptr<SimulationHasItems> m_hasItems;

	Simulation(std::wstring name = L"", Step s = 10'000 * Config::stepsPerYear);
	Simulation(std::filesystem::path path);
	Simulation(const Json& data);
	void loadAreas(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void doStep(uint16_t count = 1);
	void incrementHour();
	void save();
	void loadAreas(const Json& data, std::filesystem::path path);
	Faction& createFaction(std::wstring name);
	//TODO: latitude, longitude, altitude.
	Area& createArea(uint32_t x, uint32_t y, uint32_t z, bool createDrama = false);
	Area& loadArea(AreaId id, std::wstring name, uint32_t x, uint32_t y, uint32_t z);
	Actor& createActor(ActorParamaters params);
	Actor& createActor(const AnimalSpecies& species, Block& location, Percent percentGrown = 100);
	void destroyArea(Area& area);
	void destroyActor(Actor& actor);
	Area& loadAreaFromJson(const Json& data);
	Actor& loadActorFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Block& getBlockForJsonQuery(const Json& data);
	[[nodiscard]] Actor& getActorById(ActorId id);
	[[nodiscard]] Area& getAreaById(AreaId id) const {return *m_areasById.at(id); }
	[[nodiscard]] std::filesystem::path getPath() const  { return m_path; }
	[[nodiscard, maybe_unused]] DateTime getDateTime() const;
	~Simulation();
	// For testing.
	[[maybe_unused]] void fastForwardUntill(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAtDestination(Actor& actor, Block& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAt(Actor& actor, Block& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Actor& actor, Block& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentTo(Actor& actor, Block& block);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToHasShape(Actor& actor, HasShape& other);
	[[maybe_unused]] void fastForwardUntillActorHasNoDestination(Actor& actor);
	[[maybe_unused]] void fastForwardUntillActorHasEquipment(Actor& actor, Item& item);
	[[maybe_unused]] void fastForwardUntillPredicate(std::function<bool()> predicate, uint32_t minutes = 10);
	[[maybe_unused]] void fastForwardUntillNextEvent();
};

class HourlyEvent final : public ScheduledEvent
{
	Simulation& m_simulation;
public:
	HourlyEvent(Simulation& s, const Step start = 0) : ScheduledEvent(s, Config::stepsPerHour, start), m_simulation(s) { }
	inline void execute(){ m_simulation.incrementHour(); }
	inline void clearReferences(){ m_simulation.m_hourlyEvent.clearPointer(); }
};
