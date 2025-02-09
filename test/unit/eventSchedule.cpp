#include "../../lib/doctest.h"
#include "../../engine/eventSchedule.h"
#include "../../engine/mistDisperseEvent.h"
#include "../../engine/simulation.h"
#include "eventSchedule.hpp"
#include "reservable.h"

class TestEvent final : public ScheduledEvent
{
	HasScheduledEvent<TestEvent>* holder;
public:
	bool& fired;
	TestEvent(Step delay, bool& f, Simulation& s, HasScheduledEvent<TestEvent>* h = nullptr) : ScheduledEvent(s, delay), holder(h), fired(f) { }
	void execute(Simulation&, Area*){ fired = true; }
	void clearReferences(Simulation&, Area*) { if(holder) holder->clearPointer(); }
};
TEST_CASE("eventSchedule")
{
	Simulation simulation{L"", Step::create(1)};
	bool fired = false;
	SUBCASE("normal")
	{
		std::unique_ptr<ScheduledEvent> event = std::make_unique<TestEvent>(Step::create(10), fired, simulation);
		simulation.m_eventSchedule.schedule(std::move(event));
		CHECK(simulation.m_eventSchedule.m_data.contains(Step::create(11)));
		CHECK(simulation.m_eventSchedule.m_data.at(Step::create(11)).size() == 1);
		CHECK(!fired);
		simulation.m_eventSchedule.doStep(Step::create(11));
		CHECK(fired);
		fired = false;
		CHECK(simulation.m_eventSchedule.m_data.size() == 1);
		event = std::make_unique<TestEvent>(Step::create(10), fired, simulation);
		auto eventPtr = event.get();
		simulation.m_eventSchedule.schedule(std::move(event));
		CHECK(simulation.m_eventSchedule.m_data.contains(Step::create(11)));
		CHECK(simulation.m_eventSchedule.m_data.at(Step::create(11)).size() == 1);
		simulation.doStep();
		CHECK(eventPtr->percentComplete(simulation) == 10);
		eventPtr->cancel(simulation, nullptr);
		CHECK(eventPtr->m_cancel);
		CHECK(!fired);
		simulation.m_eventSchedule.doStep(Step::create(11));
		CHECK(!fired);
		CHECK(simulation.m_eventSchedule.m_data.size() == 1);
	}
	SUBCASE("raii fires")
	{
		HasScheduledEvent<TestEvent> holder(simulation.m_eventSchedule);
		holder.schedule(Step::create(10), fired, simulation, &holder);
		CHECK(simulation.m_eventSchedule.m_data.contains(Step::create(11)));
		CHECK(simulation.m_eventSchedule.m_data.at(Step::create(11)).size() == 1);
		CHECK(!fired);
		simulation.m_eventSchedule.doStep(Step::create(11));
		CHECK(fired);
	}
	SUBCASE("raii does not fire")
	{
		{
			HasScheduledEvent<TestEvent> holder(simulation.m_eventSchedule);
			holder.schedule(Step::create(10), fired, simulation, &holder);
			CHECK(simulation.m_eventSchedule.m_data.contains(Step::create(11)));
			CHECK(simulation.m_eventSchedule.m_data.at(Step::create(11)).size() == 1);
		}
		CHECK(!fired);
		simulation.m_eventSchedule.doStep(Step::create(11));
		CHECK(!fired);
	}
	SUBCASE("pausable holder")
	{
		HasScheduledEventPausable<TestEvent> holder(simulation.m_eventSchedule);
		holder.resume(Step::create(10), fired, simulation, &holder);
		CHECK(holder.duration() == 10);
		CHECK(simulation.m_eventSchedule.m_data.contains(Step::create(11)));
		CHECK(simulation.m_eventSchedule.m_data.at(Step::create(11)).size() == 1);
		CHECK(!fired);
		CHECK(holder.elapsedSteps() == 0);
		CHECK(!holder.isPaused());
		simulation.doStep();
		CHECK(holder.getStoredElapsedSteps() == 0);
		CHECK(holder.elapsedSteps() == 1);
		CHECK(holder.percentComplete() == 10);
		CHECK(holder.exists());
		holder.pause();
		CHECK(simulation.m_eventSchedule.m_data.at(Step::create(11)).front()->m_cancel == true);
		CHECK(holder.getStoredElapsedSteps() == 1);
		CHECK(holder.isPaused());
		CHECK(!holder.exists());
		simulation.doStep();
		holder.resume(Step::create(10), fired, simulation, &holder);
		CHECK(holder.duration() == 9);
		CHECK(simulation.m_eventSchedule.m_data.contains(Step::create(12)));
		CHECK(simulation.m_eventSchedule.m_data.at(Step::create(12)).size() == 1);
		CHECK(!fired);
		CHECK(holder.elapsedSteps() == 0);
		CHECK(holder.getStoredElapsedSteps() == 1);
		CHECK(holder.percentComplete() == 10);
		simulation.doStep();
		CHECK(holder.elapsedSteps() == 1);
		CHECK(holder.getStoredElapsedSteps() == 1);
		CHECK(holder.percentComplete() == 20);
		simulation.doStep();
		simulation.doStep();
		CHECK(holder.getStoredElapsedSteps() == 1);
		CHECK(holder.elapsedSteps() == 3);
		CHECK(holder.percentComplete() == 40);
		holder.pause();
		CHECK(holder.getStoredElapsedSteps() == 4);
		simulation.fastForward(Step::create(20));
		holder.resume(Step::create(10), fired, simulation, &holder);
		CHECK(holder.percentComplete() == 40);
		holder.pause();
		simulation.doStep();
		holder.resume(Step::create(10), fired, simulation, &holder);
		simulation.fastForward(Step::create(5));
		CHECK(holder.getStoredElapsedSteps() == 4);
		CHECK(holder.elapsedSteps() == 6);
		CHECK(holder.duration() == 6);
		CHECK(holder.percentComplete() == 100);
		CHECK(holder.getStep() == simulation.m_step);
		CHECK(!fired);
		simulation.doStep();
		CHECK(fired);
		CHECK(!holder.exists());
	}
}
