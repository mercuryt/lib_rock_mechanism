#include "actors.h"
#include "../objective.h"

void Actors::objective_addTaskToStart(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index).addTaskToStart(m_area, std::move(objective));
}
void Actors::objective_addTaskToEnd(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index).addTaskToEnd(m_area, std::move(objective));
}
void Actors::objective_replaceTasks(ActorIndex index, std::unique_ptr<Objective> objective)
{
	m_hasObjectives.at(index).replaceTasks(m_area, std::move(objective));
}
void Actors::objective_canNotCompleteSubobjective(ActorIndex index)
{
	m_hasObjectives.at(index).cannotCompleteSubobjective();
}
void Actors::objective_canNotCompleteObjective(ActorIndex index, Objective& objective)
{
	m_hasObjectives.at(index).cannotFulfillObjective(objective);
}
void Actors::objective_canNotFulfillNeed(ActorIndex index, Objective& objective)
{
	m_hasObjectives.at(index).cannotFulfillNeed(objective);
}
void Actors::objective_maybeDoNext(ActorIndex index)
{
	HasObjectives& hasObjectives = m_hasObjectives.at(index);
	if(!hasObjectives.hasCurrent())
		hasObjectives.getNext(m_area);
}
void Actors::objective_setPriority(ActorIndex index, const ObjectiveType& objectiveType, uint8_t priority)
{
	m_hasObjectives.at(index).m_prioritySet.setPriority(m_area, objectiveType, priority);
}
void Actors::objective_reset(ActorIndex index)
{
	m_hasObjectives.at(index).getCurrent().reset(m_area);
}
void Actors::objective_projectCannotReserve(ActorIndex index)
{
	Objective& objective = m_hasObjectives.at(index).getCurrent();
	objective.onProjectCannotReserve(m_area);
	objective.reset(m_area);
	m_hasObjectives.at(index).cannotCompleteSubobjective();
}
[[nodiscard]] bool Actors::objective_exists(ActorIndex index)
{
	return m_hasObjectives.at(index).hasCurrent();
}
[[nodiscard]] std::string Actors::objective_getCurrentName(ActorIndex index)
{
	return m_hasObjectives.at(index).getCurrent().name();
}
// For testing.
[[nodiscard]] bool Actors::objective_queuesAreEmpty(ActorIndex index)
{
	return m_hasObjectives.at(index).queuesAreEmpty();
}
