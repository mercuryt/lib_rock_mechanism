#include "threadedTask.h"
void ThreadedTask::cancel() { simulation::threadedTaskEngine.remove(*this); }

void ThreadedTaskEngine::readStep()
{
	for(auto& task : m_tasks)
		simulation::pool.push_task([&](){ task->readStep(); });
}
void ThreadedTaskEngine::writeStep()
{
	//TODO: Assert task queue is empty.
	for(auto& task : m_tasks)
		task->writeStep();
	m_tasks.clear();
}
void ThreadedTaskEngine::insert(std::unique_ptr<ThreadedTask>&& task)
{
	m_tasks.insert(task);
}
void ThreadedTaskEngine::remove(ThreadedTask& task)
{
	assert(std::ranges::find(m_tasks, [&](auto& t) { return &t.get() == &task; }) != m_tasks.end());
	std::erase_if(m_tasks, [&](auto& t) { return &t.get() == &task; });
}

	template<typename ...Args>
void HasThreadedTask<TaskType>::create(Args ...args)
{
	assert(m_threadedTask == nullptr);
	std::unique_ptr<ThreadedTask> task = std::make_unique<TaskType>(args...);
	m_threadedTask = task.get();
	simulation::threadedTaskEngine.insert(std::move(task));
}
void HasThreadedTask<TaskType>::cancel()
{
	assert(m_threadedTask != nullptr);
	m_threadedTask->cancel();
}
~HasThreadedTask<TaskType>::HasThreadedTask()
{
	if(m_threadedTask != nullptr)
		m_threadedTask->cancel();
}
