#pragma once

#include "../config/config.h"
#include "../datetime.h"
#include "../deserializationMemo.h"
#include "../dialogueBox.h"
#include "../eventSchedule.hpp"
#include "../faction.h"
//#include "input.h"
#include "../random.h"
#include "../threadedTask.h"
#include "../uniform.h"
#include "../definitions/shape.h"
#include "../numericTypes/types.h"
#include "../path/pathMemo.h"
#include "hasActors.h"
#include "hasItems.h"
#include "hasConstructedItemTypes.h"
#include "hasSquads.h"

#include <future>
#include <list>
#include <memory>
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
	HasScheduledEvent<HourlyEvent> m_hourlyEvent;
	Random m_random;
	//InputQueue m_inputQueue;
	SimulationHasUniforms m_hasUniforms;
	SimulationHasFactions m_hasFactions;
	SimulationHasActors m_actors;
	SimulationHasItems m_items;
	SimulationHasConstructedItemTypes m_constructedItemTypes;
	SimulationHasPathMemos m_hasPathMemos;
	SimulationHasSquads m_hasSquads;
	DialogueBoxQueue m_hasDialogues;
private:
	DeserializationMemo m_deserializationMemo;
	//TODO: What is this for?
	std::future<void> m_stepFuture;
public:
	std::string m_name;
	std::filesystem::path m_path;
	Step m_step;
	//std::unique_ptr<World> m_world;
	// Dependency injection.
	std::unique_ptr<SimulationHasAreas> m_hasAreas;
	// Drama engine must be created after hasAreas.
	std::unique_ptr<DramaEngine> m_dramaEngine;
	std::mutex m_uiReadMutex;

	Simulation(std::string name = "", Step s = DateTime(12, 150, 10'000).toSteps());
	Simulation(std::filesystem::path path);
	Simulation(const Json& data);
	Json toJson() const;
	void doStep(int count = 1);
	void incrementHour();
	void save();
	FactionId createFaction(std::string name);
	//TODO: latitude, longitude, altitude.
	[[nodiscard]] std::filesystem::path getPath() const  { return m_path; }
	[[nodiscard, maybe_unused]] DateTime getDateTime() const;
	[[nodiscard]] Step getNextStepToSimulate() const;
	[[nodiscard]] Step getNextEventStep() const;
	[[nodiscard]] Step getDelayUntillNextTimeOfDay(const Step& timeOfDay) const;
	[[nodiscard]] SimulationHasAreas& getAreas();
	[[nodiscard]] const SimulationHasAreas& getAreas() const;
	~Simulation();
	// For testing.
	[[maybe_unused]] void fastForwardUntill(DateTime now);
	[[maybe_unused]] void fastForward(Step step);
	[[maybe_unused]] void fasterForward(Step step);
	[[maybe_unused]] void fastForwardUntillActorIsAtDestination(Area& area, const ActorIndex& actor, const Point3D& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAt(Area& area, const ActorIndex& actor, const Point3D& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToDestination(Area& area, const ActorIndex& actor, const Point3D& destination);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToLocation(Area& area, const ActorIndex& actor, const Point3D& point);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToActor(Area& area, const ActorIndex& actor, const ActorIndex& other);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToItem(Area& area, const ActorIndex& actor, const ItemIndex& other);
	[[maybe_unused]] void fastForwardUntillActorIsAdjacentToPolymorphic(Area& area, const ActorIndex& actor, const ActorOrItemIndex& target);
	[[maybe_unused]] void fastForwardUntillActorHasNoDestination(Area& area, const ActorIndex& actor);
	[[maybe_unused]] void fastForwardUntillActorHasEquipment(Area& area, const ActorIndex& actor, const ItemIndex& item);
	[[maybe_unused]] void fastForwardUntillItemIsAt(Area& area, const ItemIndex& actor, const Point3D& destination);
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
