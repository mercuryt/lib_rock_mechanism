#pragma once

#include "objective.h"
#include "config.h"
#include "findsPath.h"
#include "eventSchedule.hpp"
#include "pathRequest.h"
#include "terrainFacade.h"
#include "types.h"

class Area;
class SleepEvent;
class TiredEvent;
class SleepObjective;
class Simulation;
struct DeserializationMemo;
struct AnimalSpecies;

class MustSleep final 
{
	HasScheduledEventPausable<SleepEvent> m_sleepEvent; // 2
	HasScheduledEvent<TiredEvent> m_tiredEvent; // 2
	ActorIndex m_actor;
	BlockIndex m_location = BLOCK_INDEX_MAX;
	SleepObjective* m_objective = nullptr;
	bool m_needsSleep = false;
	bool m_isAwake = true;
public:
	MustSleep(Area& area, ActorIndex a);
	MustSleep(const Json data, ActorIndex a, Simulation& s, const AnimalSpecies& species);
	Json toJson() const;
	void tired(Area& area);
	void sleep(Area& area);
	void passout(Area& area, Step duration);
	void sleep(Area& area, Step duration, bool force = false);
	void wakeUp(Area& area);
	void makeSleepObjective(Area& area);
	void wakeUpEarly(Area& area);
	void setLocation(BlockIndex block);
	void onDeath();
	void notTired(Area& area);
	void scheduleTiredEvent(Area& area);
	[[nodiscard]] bool isAwake() const { return m_isAwake; }
	[[nodiscard]] bool getNeedsSleep() const { return m_needsSleep; }
	[[nodiscard]] BlockIndex getLocation() { return m_location; }
	friend class SleepEvent;
	friend class TiredEvent;
	friend class SleepPathRequest;
	friend class SleepObjective;
	// For UI.
	[[nodiscard]] Percent getSleepPercent() const { return m_isAwake ? 0 : m_sleepEvent.percentComplete(); }
	[[nodiscard]] Percent getTiredPercent() const { return !m_isAwake ? 0 : m_tiredEvent.percentComplete(); }
	// For testing.
	[[maybe_unused, nodiscard]] bool hasTiredEvent() const { return m_tiredEvent.exists(); }
	[[maybe_unused, nodiscard]] SleepObjective* getObjective() { return m_objective; }
};
class SleepEvent final : public ScheduledEvent
{
	MustSleep& m_needsSleep;
	bool m_force = false;
public:
	SleepEvent(Simulation& simulation, const Step delay, MustSleep& ns, bool force, const Step start = 0);
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
};
class TiredEvent final : public ScheduledEvent
{
	MustSleep& m_needsSleep;
public:
	TiredEvent(Simulation& simulation, const Step delay, MustSleep& ns, const Step start = 0);
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
};
