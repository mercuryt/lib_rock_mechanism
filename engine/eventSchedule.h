/*
 * Something which can be scheduled to happen on a future step.
 * Classes derived from ScheduledEvent are expected to provide a destructor which calls HasScheduledEvent::clearPointer.
 */

#pragma once

#include "config.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"

#include <list>
#include <map>
#include <memory>
#include <cassert>

class Simulation;
class Area;

class ScheduledEvent
{
public:
	Step m_startStep;
	Step m_step;
	bool m_cancel = false;
	// If the value 0 is passed then the current step is used for start
	// Passing a differernt start is for deserializing.
	// TODO: Use Step::null() instead.
	// Default constructor exists so ProjectWorker can have a default constructor so SmallMap with ProjectWorker can be explicitly instanced.
	ScheduledEvent() { assert(false); std::unreachable(); }
	ScheduledEvent(Simulation& simulation, const Step& delay, const Step start = Step::null());
	void cancel(Simulation& simulation, Area* area);
	virtual void execute(Simulation& simulation, Area* area) = 0;
	virtual void clearReferences(Simulation& simulation, Area* area) = 0;
	virtual void onCancel(Simulation&, Area*) { }
	virtual void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, [[maybe_unused]] const HasShapeIndex& newIndex) { }
	virtual ~ScheduledEvent() = default;
	[[nodiscard]] Percent percentComplete(Simulation& simulation) const;
	[[nodiscard]] float fractionComplete(Simulation& simulation) const;
	[[nodiscard]] Step duration() const;
	[[nodiscard]] Step remaningSteps(Simulation& simulation) const;
	[[nodiscard]] Step elapsedSteps(Simulation& simulation) const;
	ScheduledEvent(const ScheduledEvent&) = delete;
	ScheduledEvent(ScheduledEvent&&) = delete;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ScheduledEvent, m_startStep, m_step, m_cancel);
};
class EventSchedule
{
	Simulation& m_simulation;
	Area* m_area = nullptr;
public:
	EventSchedule(Simulation& s, Area* area) : m_simulation(s), m_area(area) { }
	std::map<Step, std::list<std::unique_ptr<ScheduledEvent>>> m_data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent);
	void unschedule(ScheduledEvent& scheduledEvent);
	void doStep(const Step& stepNumber);
	void clear();
	[[nodiscard]] Step getNextEventStep() const;
	[[nodiscard]] Simulation& getSimulation() { return m_simulation; }
	[[nodiscard]] Area* getArea() { return m_area; }
	[[nodiscard]] Step simulationStep() const;
	// For testing.
	[[maybe_unused, nodiscard]]uint32_t count();
};
