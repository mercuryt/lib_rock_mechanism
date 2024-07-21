#include "targetedHaul.h"
#include "../targetedHaul.h"
#include "../area.h"

TargetedHaulObjective::TargetedHaulObjective(ActorIndex actor, TargetedHaulProject& p) :
	Objective(Config::targetedHaulPriority), m_project(p) 
{ 
	m_project.addWorkerCandidate(actor, *this); 
}
TargetedHaulObjective::TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_project(*static_cast<TargetedHaulProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
Json TargetedHaulObjective::toJson() const
{
	Json data = Objective::toJson();
	data["project"] = reinterpret_cast<uintptr_t>(&m_project);
	return data;
}
void TargetedHaulObjective::execute(Area&, ActorIndex actor) { m_project.commandWorker(actor); }
void TargetedHaulObjective::cancel(Area&, ActorIndex actor) { m_project.removeWorker(actor); }
