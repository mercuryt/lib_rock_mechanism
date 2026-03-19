#include "../window.h"
#include "../displayData.h"
#include "../imguiHelpers.h"
#include "screens.h"

void screens::mainMenu(Window& window)
{
	begin(window, "Goblin Pit");
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	if(window.m_simulation != nullptr)
		if(imguiButtonCentered("Continue"))
		{
			window.m_editMode = false;
			window.showGame();
		}
	if(imguiButtonCentered("Create"))
	{
		window.m_editMode = false;
		window.showCreateSimulation();
	}
	if(imguiButtonCentered("Load"))
	{
		window.m_editMode = false;
		window.showLoad();
	}
	if(imguiButtonCentered("Edit"))
	{
		window.m_editMode = true;
		if(window.m_simulation == nullptr)
			window.showLoad();
		else
			window.showEdit();
	}
	if(imguiButtonCentered("Quit"))
		window.quit();
	ImGui::PopFont();
	end();
}