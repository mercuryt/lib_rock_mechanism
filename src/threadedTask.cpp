#include "threadedTask.h"
#include "simulation.h"
#include <algorithm>
#include <memory>
#include <cassert>
void ThreadedTask::cancel() 
{ 
	m_engine.remove(*this); 
}

void ThreadedTaskEngine::readStep()
{
	for(auto& task : m_tasks)
		m_simulation.m_pool.push_task([&](){ task->readStep(); });
}
void ThreadedTaskEngine::writeStep()
{
	//TODO: Assert task queue is empty.
	for(auto& task : m_tasks)
	{
		task->clearReferences();
		task->writeStep();
	}
	m_tasks.clear();
}
void ThreadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	m_tasks.insert(std::move(task));
}
void ThreadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find_if(m_tasks, [&](auto& t) { return t.get() == &task; }) != m_tasks.end());
	task.clearReferences();
	std::erase_if(m_tasks, [&](auto& t) { return t.get() == &task; });
}
