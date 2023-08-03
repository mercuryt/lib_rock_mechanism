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
