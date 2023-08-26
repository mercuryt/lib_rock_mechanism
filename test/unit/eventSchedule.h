#include "../../lib/doctest.h"
#include "../../src/eventSchedule.h"
#include "../../src/mistDisperseEvent.h"
#include "../../src/block.h"
#include "../../src/simulation.h"

class TestEvent final : public ScheduledEvent
{
public:
	bool& fired;
	TestEvent(uint32_t delay, bool& f, Simulation& s) : ScheduledEvent(s, delay), fired(f) { }
	void execute(){ fired = true; }
	void clearReferences() { }
};
class TestEventWithPercent final : public ScheduledEventWithPercent
{
public:
	bool& fired;
	HasScheduledEvent<TestEventWithPercent>& hasScheduledEvent;
	TestEventWithPercent(uint32_t delay, bool& f, HasScheduledEvent<TestEventWithPercent>& hse, Simulation& s) : ScheduledEventWithPercent(s, delay), fired(f), hasScheduledEvent(hse) { }
	void execute(){ fired = true; }
	void clearReferences() { hasScheduledEvent.clearPointer(); }
};
TEST_CASE("event schedule")
{
	Simulation simulation;
	Block b1;
	bool fired = false;
	std::unique_ptr<ScheduledEvent> event = std::make_unique<TestEvent>(10, fired, simulation);
	simulation.m_eventSchedule.schedule(std::move(event));
	CHECK(simulation.m_eventSchedule.m_data.contains(11));
	CHECK(simulation.m_eventSchedule.m_data.at(11).size() == 1);
	CHECK(!fired);
	simulation.m_eventSchedule.execute(11);
	CHECK(fired);
	fired = false;
	CHECK(simulation.m_eventSchedule.m_data.size() == 1);
	event = std::make_unique<TestEvent>(10, fired, simulation);
	auto eventPtr = event.get();
	simulation.m_eventSchedule.schedule(std::move(event));
	CHECK(simulation.m_eventSchedule.m_data.contains(11));
	CHECK(simulation.m_eventSchedule.m_data.at(11).size() == 1);
	eventPtr->cancel();
	CHECK(eventPtr->m_cancel);
	CHECK(!fired);
	simulation.m_eventSchedule.execute(11);
	CHECK(!fired);
	CHECK(simulation.m_eventSchedule.m_data.size() == 1);
	HasScheduledEvent<TestEventWithPercent> hasScheduledEvent(simulation.m_eventSchedule);
	CHECK(!hasScheduledEvent.exists());
	hasScheduledEvent.schedule(20, fired, hasScheduledEvent, simulation);
	simulation.m_eventSchedule.execute(21);
	CHECK(fired);
	CHECK(!hasScheduledEvent.exists());
}
