#include "threadedTask.h"
#include "simulation.h"
#include <algorithm>
#include <memory>
#include <cassert>
void ThreadedTask::cancel() { threadedTaskEngine::remove(*this); }

void threadedTaskEngine::readStep()
{
	for(auto& task : m_tasks)
		simulation::pool.push_task([&](){ task->readStep(); });
}
void threadedTaskEngine::writeStep()
{
	//TODO: Assert task queue is empty.
	for(auto& task : m_tasks)
		task->writeStep();
	m_tasks.clear();
}
void threadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	m_tasks.insert(std::move(task));
}
void threadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find_if(m_tasks, [&](auto& t) { return t.get() == &task; }) != m_tasks.end());
	std::erase_if(m_tasks, [&](auto& t) { return t.get() == &task; });
}

template<class TaskType>
template<typename ...Args>
void HasThreadedTask<TaskType>::create(Args& ...args)
{
	assert(m_threadedTask == nullptr);
	std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
	m_threadedTask = task.get();
	threadedTaskEngine::insert(std::move(task));
}
template<class TaskType>
void HasThreadedTask<TaskType>::cancel()
{
	assert(m_threadedTask != nullptr);
	m_threadedTask->cancel();
}
template<class TaskType>
 bool HasThreadedTask<TaskType>::exists() const { return m_threadedTask != nullptr; }
template<class TaskType>
HasThreadedTask<TaskType>::~HasThreadedTask()
{
	if(m_threadedTask != nullptr)
		m_threadedTask->cancel();
}
