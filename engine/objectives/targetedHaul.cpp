#include "targetedHaul.h"
#include "../targetedHaul.h"
#include "../area.h"

TargetedHaulObjective(ActorIndex a, TargetedHaulProject& p) :
	Objective(a, Config::targetedHaulPriority), m_project(p) { m_project.addWorkerCandidate(m_actor, *this); }
/*
TargetedHaulObjective::TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_project(*static_cast<TargetedHaulProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
Json TargetedHaulObjective::toJson() const
{
	Json data = Objective::toJson();
	data["project"] = reinterpret_cast<uintptr_t>(&m_project);
	return data;
}
*/
void TargetedHaulObjective::reset(Area& area) { area.getActors().canReserve_clearAll(m_actor); }
void TargetedHaulObjective::execute(Area&) { m_project.commandWorker(m_actor); }
void TargetedHaulObjective::cancel(Area&) { m_project.removeWorker(m_actor); }
