/*
 * Something which can be scheduled to happen on a future step.
 * Classes derived from ScheduledEvent are expected to provide a destructor which calls HasScheduledEvent::clearPointer.
 */

#pragma once

#include "types.h"

#include <list>
#include <map>
#include <memory>
#include <cassert>

class Simulation;
class Area;

class ScheduledEvent
{
public:
	Step m_startStep = 0;
	Step m_step = 0;
	bool m_cancel = false;
	// If the value 0 is passed then the current step is used for start
	// Passing a differernt start is for deserializing.
	ScheduledEvent(Simulation& simulation, const Step delay, const Step start = 0);
	void cancel(Simulation& simulation, Area* area);
	virtual void execute(Simulation& simulation, Area* area) = 0;
	virtual void clearReferences(Simulation& simulation, Area* area) = 0;
	virtual void onCancel(Simulation&, Area*) { }
	virtual ~ScheduledEvent() = default;
	[[nodiscard]] Percent percentComplete(Simulation& simulation) const;
	[[nodiscard]] float fractionComplete(Simulation& simulation) const;
	[[nodiscard]] Step duration() const;
	[[nodiscard]] Step remaningSteps(Simulation& simulation) const;
	[[nodiscard]] Step elapsedSteps(Simulation& simulation) const;
	ScheduledEvent(const ScheduledEvent&) = delete;
	ScheduledEvent(ScheduledEvent&&) = delete;
};
class EventSchedule
{
	Simulation& m_simulation;
	Area* m_area;
	Step m_lowestStepWithEventsScheduled = 0;
public:
	EventSchedule(Simulation& s, Area* area) : m_simulation(s), m_area(area) { }
	std::map<Step, std::list<std::unique_ptr<ScheduledEvent>>> m_data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent);
	void unschedule(ScheduledEvent& scheduledEvent);
	void doStep(const Step stepNumber);
	Simulation& getSimulation() { return m_simulation; }
	Area* getArea() { return m_area; }
	[[nodiscard]] Step simulationStep() const;
	// For testing.
	[[maybe_unused, nodiscard]]uint32_t count();
};
