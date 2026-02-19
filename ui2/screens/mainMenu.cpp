#include "../window.h"
#include "../displayData.h"
#include "screens.h"

void screens::mainMenu(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	// --- Set window size and position in logical space ---
	ImVec2 windowSize = ImVec2(400, 400);
	ImVec2 centerPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	// Center window using pivot (0.5,0.5)
	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin("mainMenu", &canClose, window.m_menuWindowFlags);
	// --- Centered Title ---
	float titleWidth = ImGui::CalcTextSize("Goblin Pit").x;
	ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5f);
	ImGui::Text("Goblin Pit");
	// --- Buttons ---
	auto centerButton = [&](const char* label, auto callback)
	{
			float buttonWidth = ImGui::CalcTextSize(label).x + 20; // add padding
			ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
			if(ImGui::Button(label, ImVec2(buttonWidth, 0)))
					callback();
	};
	ImGui::Dummy(ImVec2(0, 20)); // spacing
	if(window.m_simulation != nullptr)
		centerButton("Continue", [&]() { window.m_editMode = false; window.showGame(); });
	centerButton("Create", [&]() { window.m_editMode = false; window.showCreateSimulation(); });
	centerButton("Load", [&]() { window.m_editMode = false; window.showLoad(); });
	centerButton("Edit", [&]() {
		window.m_editMode = true;
		if(window.m_simulation == nullptr)
			window.showLoad();
		else
			window.showEdit();
	});
	centerButton("Quit", [&]() { window.quit(); });
	ImGui::PopFont();
	ImGui::End();
}