#include "../../lib/doctest.h"
#include "../../engine/threadedTask.hpp"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
class TestThreadedTask final : public ThreadedTask
{
	bool& fired;
	bool& phaseIsRead;
public:
	TestThreadedTask(ThreadedTaskEngine& tte, bool& f, bool& pir) : ThreadedTask(tte), fired(f), phaseIsRead(pir) { }
	void readStep()
	{ 
		phaseIsRead = true;
	}
	void writeStep()
	{
		phaseIsRead = false;
		fired = !fired;
	}
	void clearReferences() { }
};
class TestThreadedTaskForHolder final : public ThreadedTask
{
	bool& fired;
	bool& phaseIsRead;
	HasThreadedTask<TestThreadedTaskForHolder>& hasThreadedTask;
public:
	TestThreadedTaskForHolder(ThreadedTaskEngine& tte, bool& f, bool& pir, HasThreadedTask<TestThreadedTaskForHolder>& htt) : ThreadedTask(tte), fired(f), phaseIsRead(pir), hasThreadedTask(htt) { }
	void readStep()
	{ 
		phaseIsRead = true;
	}
	void writeStep()
	{
		phaseIsRead = false;
		fired = !fired;
	}
	void clearReferences() { hasThreadedTask.clearPointer(); }
};
TEST_CASE("threadedTask")
{
	bool fired = false;
	bool phaseIsRead = true;
	Simulation simulation;
	std::unique_ptr<ThreadedTask> taskPointer = std::make_unique<TestThreadedTask>(simulation.m_threadedTaskEngine, fired, phaseIsRead);
	auto& task = *taskPointer.get();
	simulation.m_threadedTaskEngine.insert(std::move(taskPointer));
	REQUIRE(phaseIsRead);
	REQUIRE(!fired);
	SUBCASE("run")
	{
		simulation.doStep();
		REQUIRE(!phaseIsRead);
		REQUIRE(fired);
	}
	SUBCASE("cancel")
	{
		task.cancel();
		REQUIRE(simulation.m_threadedTaskEngine.count() == 0);
		simulation.doStep();
		REQUIRE(phaseIsRead);
		REQUIRE(!fired);
	}
}
TEST_CASE("hasThreadedTask")
{
	bool fired = false;
	bool phaseIsRead = true;
	Simulation simulation;
	HasThreadedTask<TestThreadedTaskForHolder> holder(simulation.m_threadedTaskEngine);
	holder.create(simulation.m_threadedTaskEngine, fired, phaseIsRead, holder);
	TestThreadedTaskForHolder& task = holder.get();
	REQUIRE(phaseIsRead);
	REQUIRE(!fired);
	REQUIRE(holder.exists());
	SUBCASE("run")
	{
		simulation.doStep();
		REQUIRE(!phaseIsRead);
		REQUIRE(fired);
		REQUIRE(!holder.exists());
	}
	SUBCASE("cancel")
	{
		task.cancel();
		REQUIRE(!holder.exists());
		simulation.doStep();
		REQUIRE(phaseIsRead);
		REQUIRE(!fired);
	}
}
TEST_CASE("holderFallsOutOfScope")
{
	bool fired = false;
	bool phaseIsRead = true;
	Simulation simulation;
	{
		HasThreadedTask<TestThreadedTaskForHolder> holder(simulation.m_threadedTaskEngine);
		holder.create(simulation.m_threadedTaskEngine, fired, phaseIsRead, holder);
	}
	REQUIRE(simulation.m_threadedTaskEngine.count() == 0);
	simulation.doStep();
	REQUIRE(!fired);
}
