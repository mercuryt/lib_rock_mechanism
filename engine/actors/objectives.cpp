#include "actors.h"
#include "../objective.h"
#include "types.h"

void Actors::objective_addNeed(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->addNeed(m_area, std::move(objective));
}
void Actors::objective_addTaskToStart(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->addTaskToStart(m_area, std::move(objective));
}
void Actors::objective_addTaskToEnd(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->addTaskToEnd(m_area, std::move(objective));
}
void Actors::objective_replaceTasks(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->replaceTasks(m_area, std::move(objective));
}
void Actors::objective_canNotCompleteSubobjective(ActorIndex index)
{
	m_hasObjectives[index]->cannotCompleteSubobjective(m_area);
}
void Actors::objective_canNotCompleteObjective(ActorIndex index, Objective& objective)
{
	m_hasObjectives[index]->cannotFulfillObjective(m_area, objective);
}
void Actors::objective_canNotFulfillNeed(ActorIndex index, Objective& objective)
{
	m_hasObjectives[index]->cannotFulfillNeed(m_area, objective);
}
void Actors::objective_maybeDoNext(ActorIndex index)
{
	HasObjectives& hasObjectives = *m_hasObjectives[index];
	if(!hasObjectives.hasCurrent())
		hasObjectives.getNext(m_area);
}
void Actors::objective_setPriority(ActorIndex index, ObjectiveTypeId objectiveType, Priority priority)
{
	m_hasObjectives[index]->m_prioritySet.setPriority(m_area, index, objectiveType, priority);
}
void Actors::objective_reset(ActorIndex index)
{
	m_hasObjectives[index]->getCurrent().reset(m_area, index);
}
void Actors::objective_projectCannotReserve(ActorIndex index)
{
	Objective& objective = m_hasObjectives[index]->getCurrent();
	objective.onProjectCannotReserve(m_area, index);
	objective.reset(m_area, index);
	m_hasObjectives[index]->cannotCompleteSubobjective(m_area);
}
void Actors::objective_complete(ActorIndex index, Objective& objective)
{
	m_hasObjectives[index]->objectiveComplete(m_area, objective);
}
void Actors::objective_subobjectiveComplete(ActorIndex index)
{
	m_hasObjectives[index]->subobjectiveComplete(m_area);
}
void Actors::objective_cancel(ActorIndex index, Objective& objective)
{
	return m_hasObjectives[index]->cancel(m_area, objective);
}
void Actors::objective_execute(ActorIndex index)
{
	return m_hasObjectives[index]->getCurrent().execute(m_area, index);
}
bool Actors::objective_exists(ActorIndex index) const
{
	return m_hasObjectives[index]->hasCurrent();
}
std::string Actors::objective_getCurrentName(ActorIndex index) const
{
	return m_hasObjectives[index]->getCurrent().name();
}
Priority Actors::objective_getPriorityFor(ActorIndex index, ObjectiveTypeId objectiveType) const
{
	return m_hasObjectives[index]->m_prioritySet.getPriorityFor(objectiveType);
}
// For testing.
bool Actors::objective_queuesAreEmpty(ActorIndex index) const
{
	return m_hasObjectives[index]->queuesAreEmpty();
}
bool Actors::objective_isOnDelay(ActorIndex index, ObjectiveTypeId objectiveTypeId) const
{
	return m_hasObjectives[index]->m_prioritySet.isOnDelay(m_area, objectiveTypeId);
}
Step Actors::objective_getDelayEndFor(ActorIndex index, ObjectiveTypeId objectiveTypeId) const
{
	return m_hasObjectives[index]->m_prioritySet.getDelayEndFor(objectiveTypeId);
}
bool Actors::objective_hasNeed(ActorIndex index, NeedType needType) const
{
	return m_hasObjectives[index]->hasNeed(needType);
}
