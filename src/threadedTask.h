#pragma once

#include <unordered_set>
#include <memory>
#include <cassert>

class ThreadedTask
{
public:
	virtual void readStep() = 0;
	virtual void writeStep() = 0;
	virtual void clearReferences() = 0;
	void cancel();
	ThreadedTask() = default;
	ThreadedTask(const ThreadedTask&) = delete;
	ThreadedTask(ThreadedTask&&) = delete;
};
// Hold and runs threaded tasks.
namespace threadedTaskEngine
{
	inline std::unordered_set<std::unique_ptr<ThreadedTask>> tasks;
	void readStep();
	void writeStep();
	void insert(std::unique_ptr<ThreadedTask>&& task);
	void remove(ThreadedTask& task);
	void clear();
	inline uint32_t count() { return tasks.size(); }
}
template<class TaskType>
class HasThreadedTask
{
	TaskType* m_threadedTask;
public:
	HasThreadedTask() : m_threadedTask(nullptr) {}
	template<typename ...Args>
	void create(Args&& ...args)
	{
		assert(m_threadedTask == nullptr);
		std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
		m_threadedTask = static_cast<TaskType*>(task.get());
		threadedTaskEngine::insert(std::move(task));
	}
	void cancel()
	{
		assert(m_threadedTask != nullptr);
		m_threadedTask->cancel();
	}
	void clearPointer() { m_threadedTask = nullptr; }
	bool exists() const { return m_threadedTask != nullptr; }
	TaskType& get() { assert(m_threadedTask != nullptr); return *m_threadedTask; }
	~HasThreadedTask()
	{
		if(m_threadedTask != nullptr)
			m_threadedTask->cancel();
	}
};

