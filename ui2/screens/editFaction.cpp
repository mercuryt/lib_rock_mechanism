#include "screens.h"
#include "../window.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/area/area.h"

void screens::editFaction(Window& window, const FactionId factionId)
{
	assert(window.m_editMode);
	window.m_paused = true;
	begin(window, "Edit Faction");
	Faction& faction = window.m_simulation->m_hasFactions.getById(factionId);
	ImGui::InputText("name", &faction.name);
	ImGui::BeginTable("allies", 2);
	for(const FactionId& allyId : faction.allies)
	{
		Faction& ally = window.m_simulation->m_hasFactions.getById(allyId);
		ImGuiText(ally.name);
		ImGui::TableNextColumn();
		if(ImGui::Button("x"))
		{
			faction.allies.erase(allyId);
			ally.allies.erase(factionId);
		}
		ImGui::TableNextRow();
	}
	ImGui::EndTable();
	if(ImGui::BeginCombo("addAlly", ""))
	{
		for(Faction& potentialAlly : window.m_simulation->m_hasFactions.getAll())
		{
			if(faction.allies.contains(potentialAlly.id) || faction.enemies.contains(potentialAlly.id))
				continue;
			if(ImGuiSelectable(faction.name))
			{
				faction.allies.insert(potentialAlly.id);
				potentialAlly.allies.insert(factionId);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::BeginTable("enemies", 2);
	for(const FactionId& enemyId : faction.allies)
	{
		Faction& enemy = window.m_simulation->m_hasFactions.getById(enemyId);
		ImGuiText(enemy.name);
		ImGui::TableNextColumn();
		if(ImGui::Button("x"))
		{
			faction.enemies.erase(enemyId);
			enemy.enemies.erase(factionId);
		}
		ImGui::TableNextRow();
	}
	ImGui::EndTable();
	if(ImGui::BeginCombo("addEnemy", ""))
	{
		for(Faction& potentialEnemy : window.m_simulation->m_hasFactions.getAll())
		{
			if(faction.enemies.contains(potentialEnemy.id) || faction.enemies.contains(potentialEnemy.id))
				continue;
			if(ImGuiSelectable(faction.name))
			{
				faction.enemies.insert(potentialEnemy.id);
				potentialEnemy.enemies.insert(factionId);
			}
		}
		ImGui::EndCombo();
	}
	if(window.m_faction == factionId)
		ImGuiText("This faction is the current player faction");
	else if(ImGui::Button("Set this faction as current player faction"))
		window.m_faction = factionId;
	if(ImGui::Button("Close"))
		window.showGame();
	end();
}