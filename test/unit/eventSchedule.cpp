#include "../../lib/doctest.h"
#include "../../engine/eventSchedule.h"
#include "../../engine/mistDisperseEvent.h"
#include "../../engine/block.h"
#include "../../engine/simulation.h"
#include "eventSchedule.hpp"
#include "reservable.h"

class TestEvent final : public ScheduledEvent
{
	HasScheduledEvent<TestEvent>* holder;
public:
	bool& fired;
	TestEvent(uint32_t delay, bool& f, Simulation& s, HasScheduledEvent<TestEvent>* h = nullptr) : ScheduledEvent(s, delay), holder(h), fired(f) { }
	void execute(){ fired = true; }
	void clearReferences() { if(holder) holder->clearPointer(); }
};
TEST_CASE("eventSchedule")
{
	Simulation simulation{L"", 1};
	Block b1;
	bool fired = false;
	SUBCASE("normal")
	{
		std::unique_ptr<ScheduledEvent> event = std::make_unique<TestEvent>(10, fired, simulation);
		simulation.m_eventSchedule.schedule(std::move(event));
		REQUIRE(simulation.m_eventSchedule.m_data.contains(11));
		REQUIRE(simulation.m_eventSchedule.m_data.at(11).size() == 1);
		REQUIRE(!fired);
		simulation.m_eventSchedule.execute(11);
		REQUIRE(fired);
		fired = false;
		REQUIRE(simulation.m_eventSchedule.m_data.size() == 1);
		event = std::make_unique<TestEvent>(10, fired, simulation);
		auto eventPtr = event.get();
		simulation.m_eventSchedule.schedule(std::move(event));
		REQUIRE(simulation.m_eventSchedule.m_data.contains(11));
		REQUIRE(simulation.m_eventSchedule.m_data.at(11).size() == 1);
		simulation.doStep();
		REQUIRE(eventPtr->percentComplete() == 10);
		eventPtr->cancel();
		REQUIRE(eventPtr->m_cancel);
		REQUIRE(!fired);
		simulation.m_eventSchedule.execute(11);
		REQUIRE(!fired);
		REQUIRE(simulation.m_eventSchedule.m_data.size() == 1);
	}
	SUBCASE("raii fires")
	{
		HasScheduledEvent<TestEvent> holder(simulation.m_eventSchedule);
		holder.schedule(10, fired, simulation, &holder);
		REQUIRE(simulation.m_eventSchedule.m_data.contains(11));
		REQUIRE(simulation.m_eventSchedule.m_data.at(11).size() == 1);
		REQUIRE(!fired);
		simulation.m_eventSchedule.execute(11);
		REQUIRE(fired);
	}
	SUBCASE("raii does not fire")
	{
		{
			HasScheduledEvent<TestEvent> holder(simulation.m_eventSchedule);
			holder.schedule(10, fired, simulation, &holder);
			REQUIRE(simulation.m_eventSchedule.m_data.contains(11));
			REQUIRE(simulation.m_eventSchedule.m_data.at(11).size() == 1);
		}
		REQUIRE(!fired);
		simulation.m_eventSchedule.execute(11);
		REQUIRE(!fired);
	}
	SUBCASE("pausable holder")
	{
		HasScheduledEventPausable<TestEvent> holder(simulation.m_eventSchedule);
		holder.resume(10, fired, simulation, &holder);
		REQUIRE(holder.duration() == 10);
		REQUIRE(simulation.m_eventSchedule.m_data.contains(11));
		REQUIRE(simulation.m_eventSchedule.m_data.at(11).size() == 1);
		REQUIRE(!fired);
		REQUIRE(holder.elapsedSteps() == 0);
		REQUIRE(!holder.isPaused());
		simulation.doStep();
		REQUIRE(holder.getStoredElapsedSteps() == 0);
		REQUIRE(holder.elapsedSteps() == 1);
		REQUIRE(holder.percentComplete() == 10);
		REQUIRE(holder.exists());
		holder.pause();
		REQUIRE(simulation.m_eventSchedule.m_data.at(11).front()->m_cancel == true);
		REQUIRE(holder.getStoredElapsedSteps() == 1);
		REQUIRE(holder.isPaused());
		REQUIRE(!holder.exists());
		simulation.doStep();
		holder.resume(10, fired, simulation, &holder);
		REQUIRE(holder.duration() == 9);
		REQUIRE(simulation.m_eventSchedule.m_data.contains(12));
		REQUIRE(simulation.m_eventSchedule.m_data.at(12).size() == 1);
		REQUIRE(!fired);
		REQUIRE(holder.elapsedSteps() == 0);
		REQUIRE(holder.getStoredElapsedSteps() == 1);
		REQUIRE(holder.percentComplete() == 10);
		simulation.doStep();
		REQUIRE(holder.elapsedSteps() == 1);
		REQUIRE(holder.getStoredElapsedSteps() == 1);
		REQUIRE(holder.percentComplete() == 20);
		simulation.doStep();
		simulation.doStep();
		REQUIRE(holder.getStoredElapsedSteps() == 1);
		REQUIRE(holder.elapsedSteps() == 3);
		REQUIRE(holder.percentComplete() == 40);
		holder.pause();
		REQUIRE(holder.getStoredElapsedSteps() == 4);
		simulation.fastForward(20);
		holder.resume(10, fired, simulation, &holder);
		REQUIRE(holder.percentComplete() == 40);
		holder.pause();
		simulation.doStep();
		holder.resume(10, fired, simulation, &holder);
		simulation.fastForward(5);
		REQUIRE(holder.getStoredElapsedSteps() == 4);
		REQUIRE(holder.elapsedSteps() == 6);
		REQUIRE(holder.duration() == 6);
		REQUIRE(holder.percentComplete() == 100);
		REQUIRE(holder.getStep() == simulation.m_step);
		REQUIRE(!fired);
		simulation.doStep();
		REQUIRE(fired);
		REQUIRE(!holder.exists());
	}
}
