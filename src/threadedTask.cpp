#include "threadedTask.h"
#include "simulation.h"
#include <algorithm>
#include <memory>
#include <cassert>
void ThreadedTask::cancel() 
{ 
	threadedTaskEngine::remove(*this); 
}

void threadedTaskEngine::readStep()
{
	for(auto& task : tasks)
		simulation::pool.push_task([&](){ task->readStep(); });
}
void threadedTaskEngine::writeStep()
{
	//TODO: Assert task queue is empty.
	for(auto& task : tasks)
	{
		task->clearReferences();
		task->writeStep();
	}
	tasks.clear();
}
void threadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	tasks.insert(std::move(task));
}
void threadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find_if(tasks, [&](auto& t) { return t.get() == &task; }) != tasks.end());
	task.clearReferences();
	std::erase_if(tasks, [&](auto& t) { return t.get() == &task; });
}
void threadedTaskEngine::clear()
{ 
	for(auto& task : tasks)
		task->clearReferences();
	tasks.clear(); 
}
