#include "input.h"
#include "actor.h"
// InputAction
InputAction::InputAction(std::unordered_set<Actor*>& actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue) :
	m_emplacementType(emplacementType), m_actors(actors)
{
	for(Actor* actor : m_actors)
		m_onDestroySubscriptions.subscribe(actor->m_onDestroy);
	std::function<void()> callback = [&](){ inputQueue.remove(m_iterator); };
	m_onDestroySubscriptions.setCallback(callback);
}
void InputAction::insertObjective(std::unique_ptr<Objective> objective, Actor& actor)
{
	switch(m_emplacementType)
	{
		case NewObjectiveEmplacementType::After:
			actor.m_hasObjectives.addTaskToEnd(std::move(objective));
			break;
		case NewObjectiveEmplacementType::Before:
			actor.m_hasObjectives.addTaskToStart(std::move(objective));
			break;
		case NewObjectiveEmplacementType::Replace:
			actor.m_hasObjectives.replaceTasks(std::move(objective));
			break;
	}
}
// InputQueue
Queue::iterator InputQueue::insert(std::unique_ptr<InputAction> action) 
{ 
	std::scoped_lock lock(m_mutex);
	m_actions.push_back(std::move(action)); 
	return action->m_iterator = m_actions.end() - 1;
}
void InputQueue::remove(Queue::iterator iterator) { m_actions.erase(iterator); }
void InputQueue::flush() 
{ 
	std::scoped_lock lock(m_mutex);
	for(auto& action : m_actions)
		action->execute();
	m_actions.clear();
}
