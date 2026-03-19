#include "screens.h"
#include "../window.h"
#include "../../engine/faction.h"

void screens::selectFactionToEdit(Window& window)
{
	assert(window.m_editMode);
	begin(window, "Select Faction To Edit");
	for(const Faction& faction : window.m_simulation->m_hasFactions.getAll())
		if(ImGui::Button(faction.name.c_str()))
			window.showEditFaction(faction.id);
	static std::string name;
	ImGui::InputText("name", &name);
	ImGui::BeginDisabled(name.empty());
	if(ImGui::Button("create"))
		window.m_gameOverlay.m_factionToEdit = window.m_simulation->m_hasFactions.createFaction(name);
	ImGui::EndDisabled();
	if(ImGui::Button("back"))
		window.showGame();
	end();
}