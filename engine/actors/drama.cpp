#include "actors.h"
#include "../area/area.h"
#include "../simulation/simulation.h"
#include "../objectives/followScript.h"

void Actors::drama_setDialog(const ActorIndex& index, const std::string& line, const Step& duration)
{
	m_dialog[index].first = line;
	if(duration.exists())
		m_dialog[index].second = m_area.m_simulation.m_step + duration;
	else
		m_dialog[index].second.clear();
}
void Actors::drama_clearDialog(const ActorIndex& index)
{
	m_dialog[index].first.clear();
}
void Actors::drama_castForRole(const ActorIndex& index, Objective& objective, const ActorIndex& objectiveOwner)
{
	assert(m_hasObjectives[index]->getCurrent().m_priority < objective.m_priority);
	assert(index != objectiveOwner);
	m_hasObjectives[index]->addNeed(m_area, std::make_unique<FollowScriptSubObjective>(objective, getReference(objectiveOwner)));
}