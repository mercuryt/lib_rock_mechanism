#include "../../lib/doctest.h"
#include "../../engine/threadedTask.hpp"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/space/space.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
class TestThreadedTask final : public ThreadedTask
{
	bool& fired;
	bool& phaseIsRead;
public:
	TestThreadedTask(bool& f, bool& pir) : fired(f), phaseIsRead(pir) { }
	void readStep(Simulation&, Area*)
	{
		phaseIsRead = true;
	}
	void writeStep(Simulation&, Area*)
	{
		phaseIsRead = false;
		fired = !fired;
	}
	void clearReferences(Simulation&, Area*) { }
};
class TestThreadedTaskForHolder final : public ThreadedTask
{
	bool& fired;
	bool& phaseIsRead;
	HasThreadedTask<TestThreadedTaskForHolder>& hasThreadedTask;
public:
	TestThreadedTaskForHolder(bool& f, bool& pir, HasThreadedTask<TestThreadedTaskForHolder>& htt) : fired(f), phaseIsRead(pir), hasThreadedTask(htt) { }
	void readStep(Simulation&, Area*)
	{
		phaseIsRead = true;
	}
	void writeStep(Simulation&, Area*)
	{
		phaseIsRead = false;
		fired = !fired;
	}
	void clearReferences(Simulation&, Area*) { hasThreadedTask.clearPointer(); }
};
TEST_CASE("threadedTask")
{
	bool fired = false;
	bool phaseIsRead = true;
	Simulation simulation;
	std::unique_ptr<ThreadedTask> taskPointer = std::make_unique<TestThreadedTask>(fired, phaseIsRead);
	auto& task = *taskPointer.get();
	simulation.m_threadedTaskEngine.insert(std::move(taskPointer));
	CHECK(phaseIsRead);
	CHECK(!fired);
	SUBCASE("run")
	{
		simulation.doStep();
		CHECK(!phaseIsRead);
		CHECK(fired);
	}
	SUBCASE("cancel")
	{
		task.cancel(simulation, nullptr);
		CHECK(simulation.m_threadedTaskEngine.count() == 0);
		simulation.doStep();
		CHECK(phaseIsRead);
		CHECK(!fired);
	}
}
TEST_CASE("hasThreadedTask")
{
	bool fired = false;
	bool phaseIsRead = true;
	Simulation simulation;
	HasThreadedTask<TestThreadedTaskForHolder> holder(simulation.m_threadedTaskEngine);
	holder.create(fired, phaseIsRead, holder);
	TestThreadedTaskForHolder& task = holder.get();
	CHECK(phaseIsRead);
	CHECK(!fired);
	CHECK(holder.exists());
	SUBCASE("run")
	{
		simulation.doStep();
		CHECK(!phaseIsRead);
		CHECK(fired);
		CHECK(!holder.exists());
	}
	SUBCASE("cancel")
	{
		task.cancel(simulation, nullptr);
		CHECK(!holder.exists());
		simulation.doStep();
		CHECK(phaseIsRead);
		CHECK(!fired);
	}
}
/*
// Changed from implicit delete to explicit enforced with assert.
TEST_CASE("holderFallsOutOfScope")
{
	bool fired = false;
	bool phaseIsRead = true;
	Simulation simulation;
	{
		HasThreadedTask<TestThreadedTaskForHolder> holder(simulation.m_threadedTaskEngine);
		holder.create(fired, phaseIsRead, holder);
	}
	CHECK(simulation.m_threadedTaskEngine.count() == 0);
	simulation.doStep();
	CHECK(!fired);
}
*/