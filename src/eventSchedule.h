/*
 * Something which can be scheduled to happen on a future step.
 * Classes derived from ScheduledEvent are expected to provide a destructor which calls HasScheduledEvent::clearPointer.
 */

#pragma once

#include <list>
#include <unordered_map>
#include <memory>
#include <cassert>

class ScheduledEvent
{
public:
	uint32_t m_step;
	ScheduledEvent(uint32_t d);
	void cancel();
	virtual ~ScheduledEvent() {}
	virtual void execute() = 0;
	uint32_t remaningSteps() const;
	ScheduledEvent(const ScheduledEvent&) = delete;
	ScheduledEvent(ScheduledEvent&&) = delete;
};
class ScheduledEventWithPercent : public ScheduledEvent
{
public:
	uint32_t m_startStep;
	ScheduledEventWithPercent(uint32_t d);
	uint32_t percentComplete() const;
};
namespace eventSchedule
{
	inline std::unordered_map<uint32_t, std::list<std::unique_ptr<ScheduledEvent>>> data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent);
	void schedule(std::unique_ptr<ScheduledEventWithPercent> scheduledEvent);
	void unschedule(const ScheduledEvent& scheduledEvent);
	void execute(uint32_t stepNumber);
	void clear();
};
