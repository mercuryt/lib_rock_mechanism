/*
 * Something which can be scheduled to happen on a future step.
 * Classes derived from ScheduledEvent are expected to provide a destructor which calls HasScheduledEvent::clearPointer.
 */

#pragma once

#include "types.h"

#include <list>
#include <unordered_map>
#include <memory>
#include <cassert>

class ScheduledEvent
{
public:
	Step m_step;
	bool m_cancel;
	ScheduledEvent(Step delay);
	void cancel();
	virtual void execute() = 0;
	virtual void clearReferences() = 0;
	Step remaningSteps() const;
	ScheduledEvent(const ScheduledEvent&) = delete;
	ScheduledEvent(ScheduledEvent&&) = delete;
};
class ScheduledEventWithPercent : public ScheduledEvent
{
public:
	Step m_startStep;
	ScheduledEventWithPercent(uint32_t d);
	uint32_t percentComplete() const;
};
namespace eventSchedule
{
	inline std::unordered_map<Step, std::list<std::unique_ptr<ScheduledEvent>>> data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent);
	void schedule(std::unique_ptr<ScheduledEventWithPercent> scheduledEvent);
	void unschedule(ScheduledEvent& scheduledEvent);
	void execute(Step stepNumber);
	void clear();
};
