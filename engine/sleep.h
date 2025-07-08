#pragma once

#include "config.h"
#include "eventSchedule.hpp"
#include "reference.h"
#include "numericTypes/types.h"

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
	ActorReference m_actor;
	Point3D m_location;
	SleepObjective* m_objective = nullptr;
	bool m_needsSleep = false;
	bool m_isAwake = true;
	bool m_force = false;
public:
	MustSleep(Area& area, const ActorIndex& a);
	MustSleep(Area& area, const Json& data, const ActorIndex& a);
	Json toJson() const;
	void tired(Area& area);
	void sleep(Area& area);
	void passout(Area& area, const Step& duration);
	void sleep(Area& area, const Step& duration, bool force = false);
	void wakeUp(Area& area);
	void makeSleepObjective(Area& area);
	void wakeUpEarly(Area& area);
	void setLocation(const Point3D& point);
	void unschedule();
	void notTired(Area& area);
	void scheduleTiredEvent(Area& area);
	void clearObjective() { m_objective = nullptr; }
	void clearSleepSpot() { m_location.clear(); }
	[[nodiscard]] bool isAwake() const { return m_isAwake; }
	[[nodiscard]] bool isTired() const { return m_needsSleep; }
	[[nodiscard]] bool getNeedsSleep() const { return m_needsSleep; }
	[[nodiscard]] Point3D getLocation() const { return m_location; }
	friend class SleepEvent;
	friend class TiredEvent;
	friend class SleepPathRequest;
	friend class SleepObjective;
	// For UI.
	[[nodiscard]] Percent getSleepPercent() const { return m_isAwake ? Percent::create(0) : m_sleepEvent.percentComplete(); }
	[[nodiscard]] Percent getTiredPercent() const { return !m_isAwake ? Percent::create(0) : m_tiredEvent.percentComplete(); }
	// For testing.
	[[maybe_unused, nodiscard]] bool hasTiredEvent() const { return m_tiredEvent.exists(); }
	[[maybe_unused, nodiscard]] SleepObjective* getObjective() { return m_objective; }
};
class SleepEvent final : public ScheduledEvent
{
	MustSleep& m_needsSleep;
public:
	SleepEvent(Simulation& simulation, const Step& delay, MustSleep& ns, const Step start = Step::null());
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
};
class TiredEvent final : public ScheduledEvent
{
	MustSleep& m_needsSleep;
public:
	TiredEvent(Simulation& simulation, const Step& delay, MustSleep& ns, const Step start = Step::null());
	void execute(Simulation&, Area*);
	void clearReferences(Simulation&, Area*);
};
