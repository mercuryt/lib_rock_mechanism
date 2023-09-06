#include "threadedTask.h"
#include "simulation.h"
#include <algorithm>
#include <memory>
#include <cassert>
void ThreadedTask::cancel() 
{ 
	clearReferences();
	m_engine.remove(*this); 
}
void ThreadedTaskEngine::readStep()
{
	for(auto& task : m_tasks)
		m_simulation.m_pool.push_task([&](){ task->readStep(); });
}
void ThreadedTaskEngine::writeStep()
{
	std::unordered_set<std::unique_ptr<ThreadedTask>> currentTasks;
	m_tasks.swap(currentTasks);
	for(auto& task : currentTasks)
	{
		task->clearReferences();
		task->writeStep();
	}
}
void ThreadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	m_tasks.insert(std::move(task));
}
void ThreadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find_if(m_tasks, [&](auto& t) { return t.get() == &task; }) != m_tasks.end());
	std::erase_if(m_tasks, [&](auto& t) { return t.get() == &task; });
}
