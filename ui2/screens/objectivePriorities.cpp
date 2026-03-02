#include "screens.h"
#include "../window.h"
#include "../../engine/actors/actors.h"
#include "../../engine/objective.h"
#include "../../engine/area/area.h"

void screens::objectivePriorities(Window& window, const ActorReference actorRef)
{
	assert(window.m_editMode);
	Actors& actors = window.m_area->getActors();
	const ActorIndex actor = actorRef.getIndex(actors.m_referenceData);
	begin(window, "Set Priorities For " + actors.getName(actor));
	ImGui::BeginTable("objectivePriorityTable", 2);
	for(auto& objectiveType : objectiveTypeData)
	{
		ImGuiText(objectiveType->name());
		ImGui::TableNextColumn();
		int value = actors.objective_getPriorityFor(actor, objectiveType->getId()).get();
		if(ImGui::InputInt("", &value))
		{
			value = std::clamp(value, 0, Priority::max().get());
			actors.objective_setPriority(actor, objectiveType->getId(), Priority::create(value));
		}
		ImGui::TableNextRow();
	}
	ImGui::EndTable();
	if(ImGui::Button("close"))
		window.showGame();
	if(ImGui::Button("details"))
		window.showActorDetails(actorRef);
	end();
}