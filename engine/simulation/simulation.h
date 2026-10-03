#pragma once

#include "../config/config.h"
#include "../datetime.h"
#include "../deserializationMemo.h"
#include "../dialogueBox.h"
#include "../eventSchedule.hpp"
#include "../faction.h"
#include "../expedition/expedition.h"
//#include "input.h"
#include "../random.h"
#include "../threadedTask.h"
#include "../uniform.h"
#include "../definitions/shape.h"
#include "../numericTypes/types.h"
#include "hasActors.h"
#include "hasItems.h"
#include "hasConstructedItemTypes.h"
#include "hasSquads.h"

#include <future>
#include <list>
#include <memory>
#include <mutex>

class HourlyEvent;
class DramaEngine;
class SimulationHasAreas;
class World;

class Simulation final
{
public:
	EventSchedule m_eventSchedule;
	ThreadedTaskEngine m_threadedTaskEngine;
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	Random m_random;
	//InputQueue m_inputQueue;
	SimulationHasUniforms m_hasUniforms;
	SimulationHasFactions m_hasFactions;
	SimulationHasActors m_actors;
	SimulationHasItems m_items;
	SimulationHasConstructedItemTypes m_constructedItemTypes;
	SimulationHasSquads m_hasSquads;
	SimulationHasExpeditions m_hasExpeditions;
	DialogueBoxQueue m_hasDialogues;
private:
	DeserializationMemo m_deserializationMemo;
	//TODO: What is this for?
	std::future<void> m_stepFuture;
public:
	std::string m_name;
	std::filesystem::path m_path;
	Step m_step;
	// Dependency injection.
	std::unique_ptr<SimulationHasAreas> m_hasAreas;
	// Drama engine must be created after hasAreas.
	std::unique_ptr<DramaEngine> m_dramaEngine;
	std::unique_ptr<World> m_world;
	std::mutex m_uiReadMutex;
	// Default dateTime provided for testing: mid day, so not too cold, 1000 years, so even the oldest living things are born at a positive numbered step.
	Simulation(const std::string& name = "", const DateTime& dateTime = DateTime(12, 160, 1000));
	Simulation(const std::string& name, const Step step);
	Simulation(std::filesystem::path path);
	Simulation(const Json& data);
	Json toJson() const;
	void doStep(int count = 1);
	void incrementHour();
	void save();
	FactionId createFaction(std::string name);
	//TODO: latitude, longitude, altitude.
	[[nodiscard]] std::filesystem::path getPath() const { return m_path; }
	[[nodiscard, maybe_unused]] DateTime getDateTime() const;
	[[nodiscard]] Step getNextStepToSimulate() const;
	[[nodiscard]] Step getNextEventStep() const;
	[[nodiscard]] Step getDelayUntillNextTimeOfDay(const Step timeOfDay) const;
	[[nodiscard]] SimulationHasAreas& getAreas();
	[[nodiscard]] const SimulationHasAreas& getAreas() const;
	~Simulation();
	// For testing.
	[[maybe_unused]] void fastForwardUntill(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fasterForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAtDestination(Area& area, ActorIndex actor, const Point3D destination);
	[[maybe_unused]] void fastForwardUntillActorIsAt(Area& area, ActorIndex actor, const Point3D destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Area& area, ActorIndex actor, const Point3D destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToLocation(Area& area, ActorIndex actor, const Point3D point);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToItem(Area& area, ActorIndex actor, const ItemIndex other);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToPolymorphic(Area& area, ActorIndex actor, const ActorOrItemIndex target);
	[[maybe_unused]] void fastForwardUntillActorHasNoDestination(Area& area, ActorIndex actor);
	[[maybe_unused]] void fastForwardUntillActorHasEquipment(Area& area, ActorIndex actor, const ItemIndex item);
	[[maybe_unused]] void fastForwardUntillItemIsAt(Area& area, const ItemIndex actor, const Point3D destination);
	[[maybe_unused]] void fastForwardUntillPredicate(std::function<bool()>&& predicate, int minutes = 10);
	[[maybe_unused]] void fastForwardUntillPredicate(std::function<bool()>& predicate, int minutes = 10);
	[[maybe_unused]] void fasterForwardUntillPredicate(std::function<bool()>& predicate, int minutes = 10);
	[[maybe_unused]] void fasterForwardUntillPredicate(std::function<bool()>&& predicate, int minutes = 10);
	[[maybe_unused]] void fastForwardUntillNextEvent();
	[[nodiscard, maybe_unused]] DeserializationMemo& getDeserializationMemo() { return m_deserializationMemo; }
	// temportary.
	friend class ScheduledEvent;
	friend class ThreadedTask;
};

class HourlyEvent final : public ScheduledEvent
{
public:
	HourlyEvent(Simulation& s, const Step start = Step::null()) : ScheduledEvent(s, Config::stepsPerHour, start) { }
	inline void execute(Simulation& simulation, Area*){ simulation.incrementHour(); }
	inline void clearReferences(Simulation& simulation, Area*){ simulation.m_hourlyEvent.clearPointer(); }
};
