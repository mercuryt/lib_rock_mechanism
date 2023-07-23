#pragma once

#include <unordered_set>
#include <memory>

class ThreadedTask
{
public:
	virtual void readStep() = 0;
	virtual void writeStep() = 0;
	void cancel();
	ThreadedTask() = default;
	virtual ~ThreadedTask() {}
	ThreadedTask(const ThreadedTask&) = delete;
	ThreadedTask(ThreadedTask&&) = delete;
};
// Hold and runs threaded tasks.
namespace threadedTaskEngine
{
	std::unordered_set<std::unique_ptr<ThreadedTask>> m_tasks;
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
	void create(Args& ...args);
	void cancel();
	bool exists() const;
	~HasThreadedTask();
};
