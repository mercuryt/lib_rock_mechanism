#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/simulation/simulation.h"

void screens::createSimulation(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	static char name[128];
	static int hour = 1;
	static int day = 1;
	static int year = 1;
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
	ImGui::SetNextWindowSize({400,400});
	ImGui::Begin("createSimulation", &canClose, window.m_menuWindowFlags);
	ImGui::Text("Create Simulation");
	ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
	ImGui::PushItemWidth(100.f);
	ImGui::InputInt("Hour", &hour);
	ImGui::InputInt("Day Of Year", &day);
	ImGui::InputInt("Year", &year);
	ImGui::PopItemWidth();
	if(hour < 1) hour = 1;
	if(day < 1) day = 1;
	if(year < 1) year = 1;
	if(hour > 24) hour = 24;
	if(day > 31) day = 31;
	if(ImGui::Button("Create") && name[0] != '\0')
	{
		window.createSimulation(std::string(name), DateTime(hour, day, year));
		window.showMainMenu();
	}
	ImGui::SameLine();
	if(ImGui::Button("Back"))
		window.showMainMenu();
	ImGui::PopFont();
	ImGui::End();
}