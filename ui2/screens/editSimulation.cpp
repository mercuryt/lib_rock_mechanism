#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"

void screens::editSimulation(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	begin(window, "Edit Simulation");
	ImGui::InputText("Name", &window.m_simulation->m_name);
	ImGui::SetNextWindowSize({400,400});
	ImGui::BeginChild("areas");
	for(const auto& [areaId, area] : window.m_simulation->m_hasAreas->getAll())
	{
		if(ImGuiButton(area->m_name))
		{
			window.m_editMode = true;
			window.setArea(*area);
			window.showGame();
		}
	}
	ImGui::EndChild();
	if(ImGui::Button("Create Area"))
		window.showCreateArea();
	if(ImGui::Button("Back"))
		window.showMainMenu();
	ImGui::PopFont();
	end();
}