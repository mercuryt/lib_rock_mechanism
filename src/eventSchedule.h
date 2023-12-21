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

class ScheduledEvent
{
public:
	Simulation& m_simulation;
	const Step m_step;
	bool m_cancel;
	ScheduledEvent(Simulation& simulation, const Step delay, const Step start);
	void cancel();
	virtual void execute() = 0;
	virtual void clearReferences() = 0;
	virtual void onCancel() { }
	[[nodiscard]] Step remaningSteps() const;

	ScheduledEvent(const ScheduledEvent&) = delete;
	ScheduledEvent(ScheduledEvent&&) = delete;
	virtual ~ScheduledEvent() = default;
};
class ScheduledEventWithPercent : public ScheduledEvent
{
public:
	Step m_startStep;
	// If no value is passed to start then the sentinal value of 0 is generated and then replaced with the current step number.
	// This is for deserializing.
	ScheduledEventWithPercent(Simulation& simulation, const Step delay, const Step start = 0);
	[[nodiscard]] Percent percentComplete() const;
	[[nodiscard]] Step duration() const;
};
class EventSchedule
{
	Simulation& m_simulation;
	Step m_lowestStepWithEventsScheduled;
public:
	EventSchedule(Simulation& s) : m_simulation(s), m_lowestStepWithEventsScheduled(0) { }
	std::map<Step, std::list<std::unique_ptr<ScheduledEvent>>> m_data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent);
	void schedule(std::unique_ptr<ScheduledEventWithPercent> scheduledEvent);
	void unschedule(ScheduledEvent& scheduledEvent);
	void execute(const Step stepNumber);
	// For testing.
	[[maybe_unused, nodiscard]]uint32_t count();
};
