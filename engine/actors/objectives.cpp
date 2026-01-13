#include "actors.h"
#include "../objective.h"
#include "numericTypes/types.h"

void Actors::objective_addNeed(const ActorIndex& index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->addNeed(m_area, std::move(objective));
}
void Actors::objective_addTaskToStart(const ActorIndex& index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->addTaskToStart(m_area, std::move(objective));
}
void Actors::objective_addTaskToEnd(const ActorIndex& index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->addTaskToEnd(m_area, std::move(objective));
}
void Actors::objective_replaceTasks(const ActorIndex& index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives[index]->replaceTasks(m_area, std::move(objective));
}
void Actors::objective_canNotCompleteSubobjective(const ActorIndex& index)
{
	m_hasObjectives[index]->cannotCompleteSubobjective(m_area);
}
void Actors::objective_canNotCompleteObjective(const ActorIndex& index, Objective& objective)
{
	m_hasObjectives[index]->cannotFulfillObjective(m_area, objective);
}
void Actors::objective_canNotFulfillNeed(const ActorIndex& index, Objective& objective)
{
	m_hasObjectives[index]->cannotFulfillNeed(m_area, objective);
}
void Actors::objective_maybeDoNext(const ActorIndex& index)
{
	HasObjectives& hasObjectives = *m_hasObjectives[index];
	if(!hasObjectives.hasCurrent())
		hasObjectives.getNext(m_area);
}
void Actors::objective_setPriority(const ActorIndex& index, const ObjectiveTypeId& objectiveType, Priority priority)
{
	m_hasObjectives[index]->m_prioritySet.setPriority(m_area, index, objectiveType, priority);
}
void Actors::objective_reset(const ActorIndex& index)
{
	m_hasObjectives[index]->getCurrent().reset(m_area, index);
}
void Actors::objective_projectCannotReserve(const ActorIndex& index)
{
	Objective& objective = m_hasObjectives[index]->getCurrent();
	objective.onProjectCannotReserve(m_area, index);
	objective.reset(m_area, index);
	m_hasObjectives[index]->cannotCompleteSubobjective(m_area);
}
void Actors::objective_complete(const ActorIndex& index, Objective& objective)
{
	m_hasObjectives[index]->objectiveComplete(m_area, objective);
}
void Actors::objective_subobjectiveComplete(const ActorIndex& index)
{
	m_hasObjectives[index]->subobjectiveComplete(m_area);
}
void Actors::objective_cancel(const ActorIndex& index, Objective& objective)
{
	return m_hasObjectives[index]->cancel(m_area, objective);
}
void Actors::objective_execute(const ActorIndex& index)
{
	return m_hasObjectives[index]->getCurrent().execute(m_area, index);
}
bool Actors::objective_exists(const ActorIndex& index) const
{
	return m_hasObjectives[index]->hasCurrent();
}
std::string Actors::objective_getCurrentName(const ActorIndex& index) const
{
	return m_hasObjectives[index]->getCurrent().name();
}
ObjectiveTypeId Actors::objective_getCurrentTypeId(const ActorIndex& index) const
{
	auto& hasObjectives = m_hasObjectives[index];
	if(hasObjectives->hasCurrent())
		return m_hasObjectives[index]->getCurrent().getTypeId();
	else
		return ObjectiveTypeId::null();
}
Priority Actors::objective_getPriorityFor(const ActorIndex& index, const ObjectiveTypeId& objectiveType) const
{
	return m_hasObjectives[index]->m_prioritySet.getPriorityFor(objectiveType);
}
// For testing.
bool Actors::objective_queuesAreEmpty(const ActorIndex& index) const
{
	return m_hasObjectives[index]->queuesAreEmpty();
}
bool Actors::objective_isOnDelay(const ActorIndex& index, const ObjectiveTypeId& objectiveTypeId) const
{
	return m_hasObjectives[index]->m_prioritySet.isOnDelay(m_area, objectiveTypeId);
}
Step Actors::objective_getDelayEndFor(const ActorIndex& index, const ObjectiveTypeId& objectiveTypeId) const
{
	return m_hasObjectives[index]->m_prioritySet.getDelayEndFor(objectiveTypeId);
}
Step Actors::objective_getNeedDelayRemaining(const ActorIndex& index, NeedType needType) const
{
	return m_hasObjectives[index]->getNeedDelayRemaining(needType);
}
bool Actors::objective_hasNeed(const ActorIndex& index, NeedType needType) const
{
	return m_hasObjectives[index]->hasNeed(needType);
}
bool Actors::objective_hasSupressedNeed(const ActorIndex& index, NeedType needType) const
{
	return m_hasObjectives[index]->hasSupressedNeed(needType);
}