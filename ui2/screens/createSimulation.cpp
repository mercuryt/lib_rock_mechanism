#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../imguiHelpers.h"
#include "../imgui/imgui_stdlib.h"
#include "../../engine/simulation/simulation.h"

void screens::createSimulation(Window& window)
{
	ImVec2 windowSize{400, 600};
	begin(window, "Create Simulation", &windowSize);
	static char name[128];
	static int hour = 12;
	static int day = 180;
	static int year = 10000;
	ImGui::InputText("Name", name, IM_ARRAYSIZE(name));
	ImGui::PushItemWidth(300.f);
	ImGui::InputInt("Hour", &hour);
	ImGui::InputInt("Day Of Year", &day);
	ImGui::InputInt("Year", &year);
	ImGui::PopItemWidth();
	if(hour < 1) hour = 1;
	if(day < 1) day = 1;
	if(year < 1) year = 1;
	if(hour > 24) hour = 24;
	if(day > 31) day = 31;
	if(imguiButtonCentered("Create") && name[0] != '\0')
	{
		window.createSimulation(std::string(name), DateTime(hour, day, year));
		window.showMainMenu();
	}
	if(imguiButtonCentered("Back"))
		window.showMainMenu();
	end();
}