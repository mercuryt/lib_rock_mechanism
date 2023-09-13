//ObjectiveTypePrioritySet
#include "objective.h"
#include "simulation.h"
#include "actor.h"
#include "stamina.h"
#include "wander.h"
void ObjectiveTypePrioritySet::setPriority(const ObjectiveType& objectiveType, uint8_t priority)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& x) { return x.objectiveType == &objectiveType; });
	if(found == m_data.end())
		m_data.emplace_back(&objectiveType, priority, 0); // waitUntillStepBeforeAssigningAgain initilizes to 0.
	else
		found->priority = priority;
	std::ranges::sort(m_data, std::ranges::greater{}, &ObjectivePriority::priority);
}
void ObjectiveTypePrioritySet::remove(const ObjectiveType& objectiveType)
{
	std::erase_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == &objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Actor& actor)
{
	assert(actor.m_hasObjectives.m_currentObjective == nullptr);
	assert(actor.m_hasObjectives.m_needsQueue.empty());
	assert(actor.m_hasObjectives.m_tasksQueue.empty());
	Step currentStep = actor.getSimulation().m_step;
	for(auto& objectivePriority : m_data)
		if(currentStep > objectivePriority.doNotAssignAginUntil && objectivePriority.objectiveType->canBeAssigned(actor))
		{
			actor.m_hasObjectives.addTaskToStart(objectivePriority.objectiveType->makeFor(actor));
			return;
		}
	// No assignable tasks, do an idle task.
	if(!actor.m_stamina.isFull())
		actor.m_hasObjectives.addTaskToStart(std::make_unique<RestObjective>(actor));
	else
		actor.m_hasObjectives.addTaskToStart(std::make_unique<WanderObjective>(actor));
}
void ObjectiveTypePrioritySet::setDelay(ObjectiveId objectiveId)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType->getObjectiveId() == objectiveId; });
	assert(found != m_data.end());
	found->doNotAssignAginUntil = m_actor.getSimulation().m_step + Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective;
}
// SupressedNeed
SupressedNeed::SupressedNeed(std::unique_ptr<Objective> o, Actor& a) : m_actor(a), m_objective(std::move(o)), m_event(a.getSimulation().m_eventSchedule) { }
void SupressedNeed::callback() 
{
	auto objective = std::move(m_objective);
	m_actor.m_hasObjectives.m_supressedNeeds.erase(objective->getObjectiveId());
       	m_actor.m_hasObjectives.addNeed(std::move(objective)); 
}
SupressedNeedEvent::SupressedNeedEvent(SupressedNeed& sn) : ScheduledEventWithPercent(sn.m_actor.getSimulation(), Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective), m_supressedNeed(sn) { }
void SupressedNeedEvent::execute() { m_supressedNeed.callback(); }
void SupressedNeedEvent::clearReferences() { m_supressedNeed.m_event.clearPointer(); }
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
			setCurrentObjective(*m_needsQueue.front().get());
		else
		{
			if(m_tasksQueue.front()->m_priority > m_needsQueue.front()->m_priority)
				setCurrentObjective(*m_tasksQueue.front().get());
			else
				setCurrentObjective(*m_needsQueue.front().get());
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
			m_currentObjective->delay();
			setCurrentObjective(objective);
		}
	}
}
void HasObjectives::setCurrentObjective(Objective& objective)
{
	m_currentObjective = &objective;
	objective.execute();
}
HasObjectives::HasObjectives(Actor& a) : m_actor(a), m_currentObjective(nullptr), m_prioritySet(a) { }
void HasObjectives::addNeed(std::unique_ptr<Objective> objective)
{
	ObjectiveId objectiveId = objective->getObjectiveId();
	if(hasSupressedNeed(objectiveId))
		return;
	// Wake up to fulfil need.
	if(!m_actor.m_mustSleep.isAwake())
		m_actor.m_mustSleep.wakeUpEarly();
	Objective* o = objective.get();
	m_needsQueue.push_back(std::move(objective));
	m_needsQueue.sort([](const std::unique_ptr<Objective>& a, const std::unique_ptr<Objective>& b){ return a->m_priority > b->m_priority; });
	m_idsOfObjectivesInNeedsQueue.insert(objectiveId);
	maybeUsurpsPriority(*o);
}
void HasObjectives::addTaskToEnd(std::unique_ptr<Objective> objective)
{
	Objective* o = objective.get();
	m_tasksQueue.push_back(std::move(objective));
	if(m_currentObjective == nullptr)
		setCurrentObjective(*o);
}
void HasObjectives::addTaskToStart(std::unique_ptr<Objective> objective)
{
	Objective* o = objective.get();
	m_tasksQueue.push_front(std::move(objective));
	maybeUsurpsPriority(*o);
}
// Called when an objective is canceled by the player, canceled by the actor, or completed sucessfully.
// TODO: seperate functions for tasks and needs?
void HasObjectives::cancel(Objective& objective)
{
	ObjectiveId objectiveId = objective.getObjectiveId();
	bool isCurrent = m_currentObjective == &objective;
	objective.cancel();
	if(m_idsOfObjectivesInNeedsQueue.contains(objectiveId))
	{
		// Remove canceled objective from needs queue.
		auto found = std::ranges::find_if(m_needsQueue, [&](auto& o){ return o->getObjectiveId() == objectiveId; });
		m_needsQueue.erase(found);
		m_idsOfObjectivesInNeedsQueue.erase(objectiveId);
	}
	else
	{
		// Remove canceled objective from task queue.
		assert(m_tasksQueue.end() != std::ranges::find_if(m_tasksQueue, [&](auto& o){ return &objective == o.get(); }));
		std::erase_if(m_tasksQueue, [&](auto& o){ return &objective == o.get(); });
	}
	m_actor.m_canReserve.clearAll();
	if(isCurrent)
		getNext();
}
void HasObjectives::objectiveComplete(Objective& objective)
{
	assert(m_actor.m_mustSleep.isAwake());
	// Response to complete is the same as response to cancel.
	cancel(objective);
}
void HasObjectives::taskComplete()
{
	assert(m_actor.m_mustSleep.isAwake());
	if(m_currentObjective == nullptr)
		m_prioritySet.setObjectiveFor(m_actor);
	else
		m_currentObjective->execute();
}
void HasObjectives::cannotCompleteTask()
{
	//TODO: generate cancelaton message?
	// Response to cannot complete is the same as response to complete.
	taskComplete();
}
Objective& HasObjectives::getCurrent() 
{
	assert(m_currentObjective != nullptr);
       	return *m_currentObjective; 
}
// Does not use ::cancel because needs to move supressed objective into storage.
void HasObjectives::cannotFulfillNeed(Objective& objective)
{
	ObjectiveId objectiveId = objective.getObjectiveId();
	bool isCurrent = m_currentObjective == &objective;
	objective.cancel();
	auto found = std::ranges::find_if(m_needsQueue, [&](std::unique_ptr<Objective>& o) { return o->getObjectiveId() == objectiveId; });
	assert(found != m_needsQueue.end());
	// Store supressed need.
	m_supressedNeeds.try_emplace(objectiveId, std::move(*found), m_actor);
	// Remove from needs queue.
	m_idsOfObjectivesInNeedsQueue.erase(objectiveId);
	m_needsQueue.erase(found);
	m_actor.m_canReserve.clearAll();
	if(isCurrent)
		getNext();
}
void HasObjectives::cannotFulfillObjective(Objective& objective)
{
	// Store delay to wait before trying again.
	m_prioritySet.setDelay(objective.getObjectiveId());
	cancel(objective);
	//TODO: generate cancelation message?
}
