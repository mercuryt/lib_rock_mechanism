#pragma once

#include <vector>
#include <memory>
class ThreadedTask;
class Simulation;
class Area;

// Hold and runs threaded tasks.
class ThreadedTaskEngine
{
public:
	std::vector<std::unique_ptr<ThreadedTask>> m_tasksForThisStep;
	std::vector<std::unique_ptr<ThreadedTask>> m_tasksForNextStep;
	void doStep(Simulation&, Area* area);
	void insert(std::unique_ptr<ThreadedTask>&& task);
	void remove(ThreadedTask& task);
	void clear(Simulation& simulation, Area* area);
	// For testing.
	[[maybe_unused, nodiscard]]inline uint32_t count() { return m_tasksForNextStep.size(); }
};
class ThreadedTask
{
public:
	// Parallel step.
	virtual void readStep(Simulation& simulation, Area* area) = 0;
	// Sequential step.
	virtual void writeStep(Simulation& simulation, Area* area) = 0;
	// To be called before write step, must at minimum call clearPointer on HasThreadedTask, if one exists.
	virtual void clearReferences(Simulation& simulation, Area* area) = 0;
	// Calls clearReferences.
	void cancel(Simulation& simulation, Area* area);
	ThreadedTask() = default;
	ThreadedTask(const ThreadedTask&) = delete;
	ThreadedTask(ThreadedTask&&) = delete;
	virtual ~ThreadedTask() = default;
};
