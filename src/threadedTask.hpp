#pragma once
#include "threadedTask.h"
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
	void cancel()
	{
		assert(m_threadedTask != nullptr);
		m_threadedTask->cancel();
	}
	void clearPointer() { m_threadedTask = nullptr; }
	bool exists() const { return m_threadedTask != nullptr; }
	void maybeCancel() { if(exists()) cancel(); }
	TaskType& get() { assert(m_threadedTask != nullptr); return *m_threadedTask; }
	// Threaded tasks must be canceled before the holder is destroyed.
	~HasThreadedTask() { maybeCancel(); }
};

