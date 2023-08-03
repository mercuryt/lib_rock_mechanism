#pragma once

#include <unordered_set>
#include <memory>
#include <cassert>

class ThreadedTask
{
public:
	virtual void readStep() = 0;
	virtual void writeStep() = 0;
	void cancel();
	ThreadedTask() = default;
	ThreadedTask(const ThreadedTask&) = delete;
	ThreadedTask(ThreadedTask&&) = delete;
};
// Hold and runs threaded tasks.
namespace threadedTaskEngine
{
	inline std::unordered_set<std::unique_ptr<ThreadedTask>> m_tasks;
	void readStep();
	void writeStep();
	void insert(std::unique_ptr<ThreadedTask>&& task);
	void remove(ThreadedTask& task);
}
template<class TaskType>
class HasThreadedTask
{
	ThreadedTask* m_threadedTask;
public:
	HasThreadedTask() : m_threadedTask(nullptr) {}
	template<typename ...Args>
	void create(Args&& ...args)
	{
		assert(m_threadedTask == nullptr);
		std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
		m_threadedTask = task.get();
		threadedTaskEngine::insert(std::move(task));
	}
	void cancel()
	{
		assert(m_threadedTask != nullptr);
		m_threadedTask->cancel();
	}
	bool exists() const { return m_threadedTask != nullptr; }
	~HasThreadedTask()
	{
		if(m_threadedTask != nullptr)
			m_threadedTask->cancel();
	}
};

