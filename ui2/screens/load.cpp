#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"

void screens::load(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	ImGui::SetNextWindowSize({400, io.DisplaySize.y * 0.9f});
	ImGui::Begin("createSimulation", &canClose, window.m_menuWindowFlags);
	float titleWidth = ImGui::CalcTextSize("Load").x;
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5);
	ImGui::Text("Load");
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
	ImGui::End();
}