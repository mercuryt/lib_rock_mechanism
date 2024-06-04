#pragma once
#include "../lib/BS_thread_pool_light.hpp"
#include "deserializationMemo.h"
#include "dialogueBox.h"
#include "shape.h"
#include "types.h"
#include "config.h"
#include "datetime.h"
#include "eventSchedule.hpp"
#include "config.h"
#include "threadedTask.h"
#include "random.h"
#include "input.h"
#include "uniform.h"
#include "faction.h"
#include <list>
#include <memory>
#include <unordered_map>
#include <mutex>

//class World;
class HourlyEvent;
class DramaEngine;
class SimulationHasItems;
class SimulationHasActors;
class SimulationHasAreas;
class Actor;
class Item;

class Simulation final
{
public:
	BS::thread_pool_light m_pool;
	EventSchedule m_eventSchedule;
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	ThreadedTaskEngine m_threadedTaskEngine;
	Random m_random;
	InputQueue m_inputQueue;
	SimulationHasUniforms m_hasUniforms;
	SimulationHasShapes m_shapes;
	SimulationHasFactions m_hasFactions;
	DialogueBoxQueue m_hasDialogues;
private:
	DeserializationMemo m_deserializationMemo;
	std::future<void> m_stepFuture;
public:
	std::vector<std::future<void>> m_taskFutures;
	std::wstring m_name;
	std::filesystem::path m_path;
	Step m_step;
	//std::unique_ptr<World> m_world;
	// Dependency injectien.
	// Items and actors are stored inside area json, so the hasItems / hasActors need to be created before hasAreas.
	std::unique_ptr<SimulationHasItems> m_hasItems;
	std::unique_ptr<SimulationHasActors> m_hasActors;
	std::unique_ptr<SimulationHasAreas> m_hasAreas;
	// Drama engine must be created after hasAreas.
	std::unique_ptr<DramaEngine> m_dramaEngine;
	std::mutex m_uiReadMutex;

	Simulation(std::wstring name = L"", Step s = 10'000 * Config::stepsPerYear);
	Simulation(std::filesystem::path path);
	Simulation(const Json& data);
	Json toJson() const;
	void doStep(uint16_t count = 1);
	void incrementHour();
	void save();
	Faction& createFaction(std::wstring name);
	//TODO: latitude, longitude, altitude.
	[[nodiscard]] BlockIndex getBlockForJsonQuery(const Json& data);
	[[nodiscard]] std::filesystem::path getPath() const  { return m_path; }
	[[nodiscard, maybe_unused]] DateTime getDateTime() const;
	~Simulation();
	// For testing.
	[[maybe_unused]] void fastForwardUntill(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAtDestination(Actor& actor, BlockIndex destination);
	[[maybe_unused]] void fastForwardUntillActorIsAt(Actor& actor, BlockIndex destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Actor& actor, BlockIndex destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentTo(Actor& actor, BlockIndex block);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToHasShape(Actor& actor, HasShape& other);
	[[maybe_unused]] void fastForwardUntillActorHasNoDestination(Actor& actor);
	[[maybe_unused]] void fastForwardUntillActorHasEquipment(Actor& actor, Item& item);
	[[maybe_unused]] void fastForwardUntillPredicate(std::function<bool()> predicate, uint32_t minutes = 10);
	[[maybe_unused]] void fastForwardUntillNextEvent();
	[[nodiscard, maybe_unused]] DeserializationMemo& getDeserializationMemo() { return m_deserializationMemo; }
};

class HourlyEvent final : public ScheduledEvent
{
	Simulation& m_simulation;
public:
	HourlyEvent(Simulation& s, const Step start = 0) : ScheduledEvent(s, Config::stepsPerHour, start), m_simulation(s) { }
	inline void execute(){ m_simulation.incrementHour(); }
	inline void clearReferences(){ m_simulation.m_hourlyEvent.clearPointer(); }
};
