#include "../../lib/doctest.h"
#include "../../src/eventSchedule.h"
#include "../../src/mistDisperseEvent.h"
#include "../../src/block.h"
#include "../../src/simulation.h"

class TestEvent final : public ScheduledEvent
{
public:
	bool& fired;
	TestEvent(uint32_t delay, bool& f) : ScheduledEvent(delay), fired(f) { }
	void execute(){ fired = true; }
	void clearReferences() { }
};
class TestEventWithPercent final : public ScheduledEventWithPercent
{
public:
	bool& fired;
	HasScheduledEvent<TestEventWithPercent>& hasScheduledEvent;
	TestEventWithPercent(uint32_t delay, bool& f, HasScheduledEvent<TestEventWithPercent>& hse) : ScheduledEventWithPercent(delay), fired(f), hasScheduledEvent(hse) { }
	void execute(){ fired = true; }
	void clearReferences() { hasScheduledEvent.clearPointer(); }
};
TEST_CASE("event schedule")
{
	simulation::init();
	Block b1;
	bool fired = false;
	std::unique_ptr<ScheduledEvent> event = std::make_unique<TestEvent>(10, fired);
	eventSchedule::schedule(std::move(event));
	CHECK(eventSchedule::data.contains(11));
	CHECK(eventSchedule::data.at(11).size() == 1);
	CHECK(!fired);
	eventSchedule::execute(11);
	CHECK(fired);
	fired = false;
	CHECK(eventSchedule::data.empty());
	event = std::make_unique<TestEvent>(10, fired);
	auto eventPtr = event.get();
	eventSchedule::schedule(std::move(event));
	CHECK(eventSchedule::data.contains(11));
	CHECK(eventSchedule::data.at(11).size() == 1);
	eventPtr->cancel();
	CHECK(eventPtr->m_cancel);
	CHECK(!fired);
	eventSchedule::execute(11);
	CHECK(!fired);
	CHECK(eventSchedule::data.empty());
	HasScheduledEvent<TestEventWithPercent> hasScheduledEvent;
	CHECK(!hasScheduledEvent.exists());
	hasScheduledEvent.schedule(20, fired, hasScheduledEvent);
	eventSchedule::execute(21);
	CHECK(fired);
	CHECK(!hasScheduledEvent.exists());
}
