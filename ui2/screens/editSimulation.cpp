#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../imguiHelpers.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"

void screens::editSimulation(Window& window)
{
	ImVec2 windowSize{400, 600};
	begin(window, "Edit Simulation", &windowSize);
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	ImGui::InputText("Name", &window.m_simulation->m_name);
	for(const auto& [areaId, area] : window.m_simulation->m_hasAreas->getAll())
	{
		if(ImGuiButton(area->m_name))
		{
			window.m_editMode = true;
			window.setArea(*area);
			window.showGame();
		}
	}
	if(imguiButtonCentered("Create Area"))
		window.showCreateArea();
	if(imguiButtonCentered("Back"))
		window.showMainMenu();
	ImGui::PopFont();
	end();
}