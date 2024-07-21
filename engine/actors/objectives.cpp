#include "actors.h"
#include "../objective.h"
#include "types.h"

void Actors::objective_addNeed(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index)->addNeed(m_area, std::move(objective));
}
void Actors::objective_addTaskToStart(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index)->addTaskToStart(m_area, std::move(objective));
}
void Actors::objective_addTaskToEnd(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index)->addTaskToEnd(m_area, std::move(objective));
}
void Actors::objective_replaceTasks(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index)->replaceTasks(m_area, std::move(objective));
}
void Actors::objective_canNotCompleteSubobjective(ActorIndex index)
{
	m_hasObjectives.at(index)->cannotCompleteSubobjective(m_area);
}
void Actors::objective_canNotCompleteObjective(ActorIndex index, Objective& objective)
{
	m_hasObjectives.at(index)->cannotFulfillObjective(m_area, objective);
}
void Actors::objective_canNotFulfillNeed(ActorIndex index, Objective& objective)
{
	m_hasObjectives.at(index)->cannotFulfillNeed(m_area, objective);
}
void Actors::objective_maybeDoNext(ActorIndex index)
{
	HasObjectives& hasObjectives = *m_hasObjectives.at(index);
	if(!hasObjectives.hasCurrent())
		hasObjectives.getNext(m_area);
}
void Actors::objective_setPriority(ActorIndex index, const ObjectiveType& objectiveType, uint8_t priority)
{
	m_hasObjectives.at(index)->m_prioritySet.setPriority(m_area, objectiveType, priority);
}
void Actors::objective_reset(ActorIndex index)
{
	m_hasObjectives.at(index)->getCurrent().reset(m_area);
}
void Actors::objective_projectCannotReserve(ActorIndex index)
{
	Objective& objective = m_hasObjectives.at(index)->getCurrent();
	objective.onProjectCannotReserve(m_area);
	objective.reset(m_area);
	m_hasObjectives.at(index)->cannotCompleteSubobjective(m_area);
}
void Actors::objective_complete(ActorIndex index, Objective& objective)
{
	m_hasObjectives.at(index)->objectiveComplete(m_area, objective);
}
void Actors::objective_subobjectiveComplete(ActorIndex index)
{
	m_hasObjectives.at(index)->subobjectiveComplete(m_area);
}
void Actors::objective_cancel(ActorIndex index, Objective& objective)
{
	return m_hasObjectives.at(index)->cancel(m_area, objective);
}
void Actors::objective_execute(ActorIndex index)
{
	return m_hasObjectives.at(index)->getCurrent().execute(m_area);
}
[[nodiscard]] bool Actors::objective_exists(ActorIndex index) const
{
	return m_hasObjectives.at(index)->hasCurrent();
}
[[nodiscard]] std::string Actors::objective_getCurrentName(ActorIndex index) const
{
	return m_hasObjectives.at(index)->getCurrent().name();
}
// For testing.
bool Actors::objective_queuesAreEmpty(ActorIndex index) const
{
	return m_hasObjectives.at(index)->queuesAreEmpty();
}
bool Actors::objective_isOnDelay(ActorIndex index, ObjectiveTypeId objectiveTypeId) const
{
	return m_hasObjectives.at(index)->m_prioritySet.isOnDelay(m_area, objectiveTypeId);
}
Step Actors::objective_getDelayEndFor(ActorIndex index, ObjectiveTypeId objectiveTypeId) const
{
	return m_hasObjectives.at(index)->m_prioritySet.getDelayEndFor(objectiveTypeId);
}
