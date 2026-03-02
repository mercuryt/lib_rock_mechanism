#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"

void screens::load(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	begin(window, "Load");
	for(const auto& [name, path] : window.m_simulationList)
	{
		float nameWidth = ImGui::CalcTextSize(name.c_str()).x;
		ImGui::SetCursorPosX((windowSize.x - nameWidth) * 0.5);
		if(ImGui::Button(name.c_str()))
			window.load(path);
	}
	if(ImGui::Button("Back"))
		window.showMainMenu();
	ImGui::PopFont();
	end();
}