#include "threadedTask.h"
#include "area.h"
#include "simulation.h"
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
	m_tasksForThisStep = std::move(m_tasksForNextStep);
	m_tasksForNextStep.clear();
	std::vector<ThreadedTask*> taskPointers;
	std::ranges::transform(m_tasksForThisStep, std::back_inserter(taskPointers), [](const std::unique_ptr<ThreadedTask>& task) { return task.get(); });
	// TODO: Batching, adaptive batching.
	auto task = [&simulation, area](ThreadedTask*& task){ task->readStep(simulation, area); };
	simulation.parallelizeTask(taskPointers, (uint16_t)Config::threadedTaskBatchSize, task);
	for(auto& task : taskPointers)
	{
		task->clearReferences(simulation, area);
		task->writeStep(simulation, area);
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
