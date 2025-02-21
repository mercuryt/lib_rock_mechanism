#include "threadedTask.h"
#include "area/area.h"
#include "simulation/simulation.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <cassert>
void ThreadedTask::cancel(Simulation& simulation, Area* area)
{
	clearReferences(simulation, area);
	if(area == nullptr)
		simulation.m_threadedTaskEngine.remove(*this);
	else
		area->m_threadedTaskEngine.remove(*this);
}
void ThreadedTaskEngine::doStep(Simulation& simulation, Area* area)
{
	m_tasksForThisStep.swap(m_tasksForNextStep);
	m_tasksForNextStep.clear();
	//#pragma omp parallel for
		for(const auto& task : m_tasksForThisStep)
			task.get()->readStep(simulation, area);
	for(auto& task : m_tasksForThisStep)
	{
		task->clearReferences(simulation, area);
		task->writeStep(simulation, area);
	}
}
void ThreadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	m_tasksForNextStep.push_back(std::move(task));
}
void ThreadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find_if(m_tasksForNextStep, [&](auto& t) { return t.get() == &task; }) != m_tasksForNextStep.end());
	std::erase_if(m_tasksForNextStep, [&](auto& t) { return t.get() == &task; });
}
void ThreadedTaskEngine::clear(Simulation& simulation, Area* area)
{
	for(auto& task : m_tasksForNextStep)
		task->clearReferences(simulation, area);
	m_tasksForNextStep.clear();
}
