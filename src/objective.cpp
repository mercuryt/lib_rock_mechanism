//ObjectiveTypePrioritySet
#include "objective.h"
#include "actor.h"
void ObjectiveTypePrioritySet::setPriority(const ObjectiveType& objectiveType, uint8_t priority)
{
	auto found = std::ranges::find_if(m_data, [&](auto& pair) { return pair.first == &objectiveType; });
	if(found == m_data.end())
		m_data.emplace_back(&objectiveType, priority);
	else
		found->second = priority;
	// Sort by priority stored in m_map, high to low.
	std::ranges::sort(m_data, [&](auto& a, auto& b){ return a.second > b.second; });
}
void ObjectiveTypePrioritySet::remove(const ObjectiveType& objectiveType)
{
	std::erase_if(m_data, [&](auto& pair){ return pair.first == &objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Actor& actor)
{
	assert(actor.m_hasObjectives.m_currentObjective == nullptr);
	assert(actor.m_hasObjectives.m_needsQueue.empty());
	assert(actor.m_hasObjectives.m_tasksQueue.empty());
	for(auto& pair : m_data)
		if(pair.first->canBeAssigned(actor))
		{
			actor.m_hasObjectives.addTaskToStart(pair.first->makeFor(actor));
			return;
		}
	// There should always be some objective avalible, even if it's wandering around or resting.
	assert(false);
}
// Objective.
Objective::Objective(uint32_t p) : m_priority(p) {}
// HasObjectives.
void HasObjectives::getNext()
{
	m_currentObjective = nullptr;
	if(m_needsQueue.empty())
	{
		if(m_tasksQueue.empty())
			m_prioritySet.setObjectiveFor(m_actor);
		else
			setCurrentObjective(*m_tasksQueue.front().get());
	}
	else
	{
		if(m_tasksQueue.empty())
			setCurrentObjective(*m_needsQueue.begin()->second.get());
		else
		{
			if(m_tasksQueue.front()->m_priority > m_needsQueue.begin()->second->m_priority)
				setCurrentObjective(*m_tasksQueue.front().get());
			else
				setCurrentObjective(*m_needsQueue.begin()->second.get());
		}
	}
}
void HasObjectives::maybeUsurpsPriority(Objective& objective)
{
	if(m_currentObjective == nullptr)
		setCurrentObjective(objective);
	else
	{
		if(m_currentObjective->m_priority < objective.m_priority)
		{
			m_currentObjective->cancel();
			setCurrentObjective(objective);
		}
	}
}
void HasObjectives::setCurrentObjective(Objective& objective)
{
	m_currentObjective = &objective;
	objective.execute();
}
HasObjectives::HasObjectives(Actor& a) : m_actor(a), m_currentObjective(nullptr) { }
void HasObjectives::addNeed(std::unique_ptr<Objective>&& objective)
{
	Objective& o = *objective.get();
	m_needsQueue.emplace_back(objective->m_priority, std::move(objective));
	maybeUsurpsPriority(o);
}
void HasObjectives::addTaskToEnd(std::unique_ptr<Objective>&& objective)
{
	Objective& o = *objective.get();
	m_tasksQueue.push_back(std::move(objective));
	if(m_currentObjective == nullptr)
		setCurrentObjective(o);
}
void HasObjectives::addTaskToStart(std::unique_ptr<Objective>&& objective)
{
	Objective& o = *objective.get();
	m_tasksQueue.push_front(std::move(objective));
	maybeUsurpsPriority(o);
}
void HasObjectives::cancel(Objective& objective)
{
	objective.cancel();
	auto found = std::find_if(m_needsQueue.begin(), m_needsQueue.end(), [&](auto& o){ return &objective == o.second.get(); });
	if(found != m_needsQueue.end())
		m_needsQueue.erase(found);
	else
		std::erase_if(m_tasksQueue, [&](auto& o){ return &objective == o.get(); });
	m_actor.m_canReserve.clearAll();
	if(m_currentObjective == &objective)
		getNext();
}
void HasObjectives::objectiveComplete(Objective& objective)
{
	cancel(objective);
}
void HasObjectives::taskComplete()
{
	m_currentObjective->execute();
}
void HasObjectives::cannotCompleteTask()
{
	//TODO: generate cancelaton message?
	m_currentObjective->execute();
}
Objective& HasObjectives::getCurrent() 
{
	assert(m_currentObjective != nullptr);
       	return *m_currentObjective; 
}
void HasObjectives::cannotFulfillNeed(Objective& objective)
{
	(void)objective;
	// There is no way to survive in this area. Try to go to another one.
	m_actor.m_canMove.tryToExitArea();
}
void HasObjectives::cannotFulfillObjective(Objective& objective)
{
	cancel(objective);
	//TODO: generate cancelation message?
}
void HasObjectives::wait(uint32_t delay) { m_waitEvent.schedule(delay, m_actor); }
void WaitEvent::execute() { m_actor.m_hasObjectives.taskComplete(); }
WaitEvent::~WaitEvent() { m_actor.m_hasObjectives.m_waitEvent.clearPointer(); }
