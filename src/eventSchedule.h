/*
 * Something which can be scheduled to happen on a future step.
 * Classes derived from ScheduledEvent are expected to provide a destructor which calls HasScheduledEvent::clearPointer.
 */

#pragma once

#include <list>
#include <unordered_map>
#include <memory>

class ScheduledEvent
{
public:
	uint32_t m_step;
	ScheduledEvent(uint32_t d);
	void cancel();
	virtual ~ScheduledEvent() {}
	virtual void execute() = 0;
	uint32_t remaningSteps();
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
	std::unordered_map<uint32_t, std::list<std::unique_ptr<ScheduledEvent>>> data;
	void schedule(std::unique_ptr<ScheduledEvent> scheduledEvent);
	void unschedule(const ScheduledEvent& scheduledEvent);
	void execute(uint32_t stepNumber);
	void clear();
};
template<class EventType>
class HasScheduledEvent
{
	ScheduledEventWithPercent* m_event;
public:
	HasScheduledEvent() : m_event(nullptr) {}
	template<typename ...Args>
	void schedule(Args ...args);
	void unschedule();
	void maybeUnschedule();
	uint32_t percentComplete() const;
	bool exists();
	void clearPointer();
	~HasScheduledEvent();
};
template<class EventType>
class HasScheduledEventPausable : public HasScheduledEvent<EventType>
{
	uint32_t m_percent;
public:
	HasScheduledEventPausable() : m_percent(0) { }
	uint32_t percentComplete() const;
	void pause();
	void reset();
	template<typename ...Args>
	void schedule(uint32_t delay, Args ...args);
};
