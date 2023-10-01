#pragma once

#include <unordered_set>
#include <memory>
#include <cassert>
class ThreadedTask;
class Simulation;

// Hold and runs threaded tasks.
class ThreadedTaskEngine
{
	Simulation& m_simulation;
public:
	ThreadedTaskEngine(Simulation& s) : m_simulation(s) { }
	std::unordered_set<std::unique_ptr<ThreadedTask>> m_tasksForThisStep;
	std::unordered_set<std::unique_ptr<ThreadedTask>> m_tasksForNextStep;
	void readStep();
	void writeStep();
	void insert(std::unique_ptr<ThreadedTask>&& task);
	void remove(ThreadedTask& task);
	void clear() { m_tasksForNextStep.clear(); }
	// For testing.
	[[maybe_unused, nodiscard]]inline uint32_t count() { return m_tasksForNextStep.size(); }
};
class ThreadedTask
{
	ThreadedTaskEngine& m_engine;
public:
	// Parallel step.
	virtual void readStep() = 0;
	// Sequential step.
	virtual void writeStep() = 0;
	// To be called before write step, must at minimum call clearPointer on HasThreadedTask, if one exists.
	virtual void clearReferences() = 0;
	// Calls clearReferences.
	void cancel();
	ThreadedTask(ThreadedTaskEngine& tte) : m_engine(tte) { }
	ThreadedTask(const ThreadedTask&) = delete;
	ThreadedTask(ThreadedTask&&) = delete;
	virtual ~ThreadedTask() = default;
};
