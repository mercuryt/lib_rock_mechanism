#pragma once
#include "threadedTask.h"
#include "types.h"
#include <vector>
template<class TaskType>
class HasThreadedTask final
{
	ThreadedTaskEngine& m_engine;
	TaskType* m_threadedTask;
public:
	HasThreadedTask(ThreadedTaskEngine& tte) : m_engine(tte), m_threadedTask(nullptr)  { }
	template<typename ...Args>
	void create(Args&& ...args)
	{
		assert(m_threadedTask == nullptr);
		std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
		m_threadedTask = static_cast<TaskType*>(task.get());
		m_engine.insert(std::move(task));
	}
	void cancel(Simulation& simulation, Area* area)
	{
		assert(m_threadedTask != nullptr);
		m_threadedTask->cancel(simulation, area);
		assert(m_threadedTask == nullptr);
	}
	void clearPointer() { m_threadedTask = nullptr; }
	bool exists() const { return m_threadedTask != nullptr; }
	void maybeCancel(Simulation& simulation, Area* area) { if(exists()) cancel(simulation, area); }
	TaskType& get() { assert(m_threadedTask != nullptr); return *m_threadedTask; }
	const TaskType& get() const { assert(m_threadedTask != nullptr); return *m_threadedTask; }
	// Threaded tasks must be canceled before the holder is destroyed.
	~HasThreadedTask() { assert(!exists()); }
};
template<class TaskType>
class HasThreadedTasks final
{
	ThreadedTaskEngine& m_engine;
	std::vector<TaskType*> m_threadedTask;
public:
	HasThreadedTasks(ThreadedTaskEngine& tte) : m_engine(tte) { }
	template<typename ...Args>
	void create(HasShapeIndex index, Args&& ...args)
	{
		assert(m_threadedTask == nullptr);
		std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
		m_threadedTask.at(index) = static_cast<TaskType*>(task.get());
		m_engine.insert(std::move(task));
	}
	void cancel(HasShapeIndex index, Simulation& simulation, Area* area)
	{
		assert(m_threadedTask.at(index) != nullptr);
		m_threadedTask.at(index)->cancel(simulation, area);
		assert(m_threadedTask.at(index) == nullptr);
	}
	void clearPointer(HasShapeIndex index) { m_threadedTask.at(index) = nullptr; }
	bool exists(HasShapeIndex index) const { return m_threadedTask.at(index) != nullptr; }
	void maybeCancel(HasShapeIndex index, Simulation& simulation, Area* area) { if(exists(index)) cancel(index, simulation, area); }
	TaskType& get(HasShapeIndex index) { assert(m_threadedTask.at(index) != nullptr); return *m_threadedTask.at(index); }
	const TaskType& get(HasShapeIndex index) const { assert(m_threadedTask.at(index) != nullptr); return *m_threadedTask.at(index); }
	// Threaded tasks must be canceled before the holder is destroyed.
	//~HasThreadedTasks() { maybeCancel(); }
};
