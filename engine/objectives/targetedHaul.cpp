#include "targetedHaul.h"
#include "../targetedHaul.h"
#include "../area.h"
#include "../actors/actors.h"

TargetedHaulObjective::TargetedHaulObjective(TargetedHaulProject& p) :
	Objective(Config::targetedHaulPriority), m_project(p) { }
TargetedHaulObjective::TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_project(*static_cast<TargetedHaulProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
Json TargetedHaulObjective::toJson() const
{
	Json data = Objective::toJson();
	data["project"] = reinterpret_cast<uintptr_t>(&m_project);
	return data;
}
void TargetedHaulObjective::execute(Area& area, const ActorIndex& actor)
{
	// Add worker here rather then constructor because set current objective clears the project.
	if(!area.getActors().project_exists(actor)) 
		m_project.addWorkerCandidate(actor, *this);
	m_project.commandWorker(actor);
}
void TargetedHaulObjective::cancel(Area&, const ActorIndex& actor) { m_project.removeWorker(actor); }
