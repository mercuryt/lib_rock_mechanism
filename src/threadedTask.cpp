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
	m_tasksForThisStep = std::move(m_tasksForNextStep);
	m_tasksForNextStep.clear();
	// TODO: Batching, adaptive batching.
	for(auto& task : m_tasksForThisStep)
		m_simulation.m_pool.push_task([&](){ task->readStep(); });
}
void ThreadedTaskEngine::writeStep()
{
	for(auto& task : m_tasksForThisStep)
	{
		task->clearReferences();
		task->writeStep();
	}
}
void ThreadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	m_tasksForNextStep.insert(std::move(task));
}
void ThreadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find_if(m_tasksForNextStep, [&](auto& t) { return t.get() == &task; }) != m_tasksForNextStep.end());
	std::erase_if(m_tasksForNextStep, [&](auto& t) { return t.get() == &task; });
}
