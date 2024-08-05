#pragma once
#include "../lib/BS_thread_pool_light.hpp"

#include "config.h"
#include "datetime.h"
#include "deserializationMemo.h"
#include "dialogueBox.h"
#include "eventSchedule.hpp"
#include "faction.h"
#include "input.h"
#include "random.h"
#include "shape.h"
#include "simulation/hasActors.h"
#include "simulation/hasItems.h"
#include "threadedTask.h"
#include "types.h"
#include "uniform.h"
#include "pathMemo.h"

#include <list>
#include <memory>
#include <unordered_map>
#include <mutex>

//class World;
class HourlyEvent;
class DramaEngine;
class SimulationHasAreas;

class Simulation final
{
public:
	EventSchedule m_eventSchedule;
	ThreadedTaskEngine m_threadedTaskEngine;
	BS::thread_pool_light m_pool;
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	Random m_random;
	InputQueue m_inputQueue;
	SimulationHasUniforms m_hasUniforms;
	SimulationHasShapes m_shapes;
	SimulationHasFactions m_hasFactions;
	SimulationHasActors m_actors;
	SimulationHasItems m_items;
	SimulationHasPathMemos m_hasPathMemos;
	DialogueBoxQueue m_hasDialogues;
private:
	DeserializationMemo m_deserializationMemo;
	std::future<void> m_stepFuture;
public:
	std::wstring m_name;
	std::filesystem::path m_path;
	Step m_step;
	//std::unique_ptr<World> m_world;
	// Dependency injection.
	std::unique_ptr<SimulationHasAreas> m_hasAreas;
	// Drama engine must be created after hasAreas.
	std::unique_ptr<DramaEngine> m_dramaEngine;
	std::mutex m_uiReadMutex;

	Simulation(std::wstring name = L"", Step s = Config::stepsPerYear * 10'000u);
	Simulation(std::filesystem::path path);
	Simulation(const Json& data);
	Json toJson() const;
	void doStep(uint16_t count = 1);
	template<class Data, class Action>
	void parallelizeTask(Data& data, uint32_t stepSize, Action& task)
	{
		auto start = data.begin();
		auto end = start + std::min(stepSize, (uint32_t)data.size());
		while(start != end)
		{
			m_pool.push_task([&task, start, end]() mutable {
				while(start != end)
				{
					task(*start);
					start++;
				}
			});
			start = end;
			end = std::min(data.end(), start + stepSize);
		}
		//TODO: Use OpenMP.
		m_pool.wait_for_tasks();
	}
	template<class Action>
	void parallelizeTaskIndices(uint32_t size, uint32_t stepSize, Action& task)
	{
		uint32_t start = 0;
		uint32_t end = start + std::min(stepSize, size);
		while(start != size)
		{
			m_pool.push_task([&task, start, end]() mutable {
				while(start != end)
				task(start++);
			});
			start = end;
			end = std::min(size, start + stepSize);
		}
		m_pool.wait_for_tasks();
	}
	void incrementHour();
	void save();
	Faction& createFaction(std::wstring name);
	//TODO: latitude, longitude, altitude.
	[[nodiscard]] std::filesystem::path getPath() const  { return m_path; }
	[[nodiscard, maybe_unused]] DateTime getDateTime() const;
	[[nodiscard]] ItemId nextItemId();
	~Simulation();
	// For testing.
	[[maybe_unused]] void fastForwardUntill(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAtDestination(Area& area, ActorIndex actor, BlockIndex destination);
	[[maybe_unused]] void fastForwardUntillActorIsAt(Area& area, ActorIndex actor, BlockIndex destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Area& area, ActorIndex actor, BlockIndex destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToLocation(Area& area, ActorIndex actor, BlockIndex block);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToItem(Area& area, ActorIndex actor, ItemIndex other);
	[[maybe_unused]] void fastForwardUntillActorHasNoDestination(Area& area, ActorIndex actor);
	[[maybe_unused]] void fastForwardUntillActorHasEquipment(Area& area, ActorIndex actor, ItemIndex item);
	[[maybe_unused]] void fastForwardUntillPredicate(std::function<bool()> predicate, uint32_t minutes = 10);
	[[maybe_unused]] void fastForwardUntillNextEvent();
	[[nodiscard, maybe_unused]] DeserializationMemo& getDeserializationMemo() { return m_deserializationMemo; }
	// temportary.
	friend class ScheduledEvent;
	friend class ThreadedTask;
};

class HourlyEvent final : public ScheduledEvent
{
public:
	HourlyEvent(Simulation& s, const Step start = Step::create(0)) : ScheduledEvent(s, Config::stepsPerHour, start) { }
	inline void execute(Simulation& simulation, Area*){ simulation.incrementHour(); }
	inline void clearReferences(Simulation& simulation, Area*){ simulation.m_hourlyEvent.clearPointer(); }
};
