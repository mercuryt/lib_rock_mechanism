#pragma once

#include <unordered_set>
class ThreadedTaskEngine;
// Base class. Child classes are expected to provide a destructor which calls setNull on HasThreadedTask.
class ThreadedTask
{
	virtual void readStep() = 0;
	virtual void writeStep() = 0;
	void cancel();
	virtual ~ThreadedTask() {}
};
// Hold and runs threaded tasks.
class ThreadedTaskEngine
{
	std::unordered_set<std::unique_ptr<ThreadedTask>> m_tasks;
	void readStep()
	{
		for(auto& task : m_tasks)
			::s_pool.push_task([&](){ task.readStep(); });
	}
	void writeStep()
	{
		//TODO: Assert task queue is empty.
		for(auto& task : m_tasks)
			task.writeStep();
		m_tasks.clear();
	}
	void insert(std::unique_ptr<ThreadedTask>&& task)
	{
		m_tasks.insert(task);
	}
	void remove(ThreadedTask& task)
	{
		assert(std::ranges::find(m_tasks, [&](auto& t) { return &t.get() == &task; }) != m_tasks.end());
		std::erase_if(m_tasks, [&](auto& t) { return &t.get() == &task; }
	}
};
void ThreadedTask::cancel{ m_engine.remove(*this); }
// There is only a single threaded task engine.
static ThreadedTaskEngine s_threadedTaskEngine;
template<class TaskType>
class HasThreadedTask
{
	ThreadedTask* m_threadedTask;
	HasThreadedTask() : m_threadedTask(nullptr) {}
public:
	template<typename ...Args>
	void create(Args ...args)
	{
		assert(m_threadedTask == nullptr);
		std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
		m_threadedTask = task.get();
		::s_threadedTaskEngine.insert(std::move(task));
	}
	void cancel()
	{
		assert(m_threadedTask != nullptr);
		m_threadedTask->cancel();
	}
	// To be called by the destructior of classes derived from ThreadedTask to prevent a dangling pointer.
	void setNull()
	{
		assert(m_threadedTask != nullptr);
		m_threadedTask = nullptr;
	}
	~HasThreadedTask()
	{
		if(m_threadedTask != nullptr)
			m_threadedTask->cancel();
	}
};
