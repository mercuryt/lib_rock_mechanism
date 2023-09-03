#include "../../lib/doctest.h"
#include "../../src/threadedTask.hpp"
#include "../../src/simulation.h"
class TestThreadedTask final : public ThreadedTask
{
	bool& fired;
public:
	bool phaseIsRead;
	TestThreadedTask(ThreadedTaskEngine& tte, bool& f) : ThreadedTask(tte), fired(f), phaseIsRead(true) { }
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
TEST_CASE("threadedTask")
{
	bool fired = false;
	Simulation simulation;
	TestThreadedTask task(simulation.m_threadedTaskEngine, fired);
	REQUIRE(task.phaseIsRead);
	REQUIRE(!fired);
	SUBCASE("run")
	{
		simulation.doStep();
		REQUIRE(!task.phaseIsRead);
		REQUIRE(fired);
	}
	SUBCASE("cancel")
	{
		task.cancel();
		REQUIRE(simulation.m_threadedTaskEngine.count() == 0);
		simulation.doStep();
		REQUIRE(!task.phaseIsRead);
		REQUIRE(fired);
	}
}
TEST_CASE("hasThreadedTask")
{
	bool fired = false;
	Simulation simulation;
	HasThreadedTask<TestThreadedTask> holder(simulation.m_threadedTaskEngine);
	TestThreadedTask& task = holder.get();
	holder.create(simulation.m_threadedTaskEngine, fired);
	REQUIRE(task.phaseIsRead);
	REQUIRE(!fired);
	REQUIRE(holder.exists());
	SUBCASE("run")
	{
		simulation.doStep();
		REQUIRE(!task.phaseIsRead);
		REQUIRE(fired);
		REQUIRE(!holder.exists());
	}
	SUBCASE("cancel")
	{
		task.cancel();
		REQUIRE(!holder.exists());
		simulation.doStep();
		REQUIRE(task.phaseIsRead);
		REQUIRE(!fired);
	}
}
TEST_CASE("holderFallsOutOfScope")
{
	bool fired = false;
	Simulation simulation;
	{
		HasThreadedTask<TestThreadedTask> holder(simulation.m_threadedTaskEngine);
		holder.create(simulation.m_threadedTaskEngine, fired);
	}
	REQUIRE(simulation.m_threadedTaskEngine.count() == 0);
	simulation.doStep();
	REQUIRE(!fired);
}
