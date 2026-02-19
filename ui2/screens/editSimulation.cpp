#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"

void screens::editSimulation(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	static char name[128];
	strcpy(name, window.m_simulation->m_name.c_str());
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	ImGui::SetNextWindowSize({400,800});
	ImGui::Begin("editSimulation", &canClose, window.m_menuWindowFlags);
	ImGui::Text("Edit Simulation");
	ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
	if(ImGui::Button("Rename") && name[0] != '\0')
		window.m_simulation->m_name = std::string(name);
	ImGui::SetNextWindowSize({400,400});
	ImGui::BeginChild("areas");
	for(const auto& [areaId, area] : window.m_simulation->m_hasAreas->getAll())
	{
		if(ImGui::Button(area->m_name.c_str()))
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
	ImGui::End();
}