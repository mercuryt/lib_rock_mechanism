#include "screens.h"
#include "../window.h"
#include "../displayData.h"
#include "../imguiHelpers.h"
#include "../../engine/simulation/simulation.h"

void screens::load(Window& window)
{
	begin(window, "Load");
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	for(const auto& [name, path] : window.m_simulationList)
	{
		if(imguiButtonCentered(name.c_str()))
			window.load(path);
	}
	if(imguiButtonCentered("Back"))
		window.showMainMenu();
	ImGui::PopFont();
	end();
}