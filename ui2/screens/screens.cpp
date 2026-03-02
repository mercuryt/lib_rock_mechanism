#include "screens.h"
#include "../window.h"
void screens::begin(Window& window, const std::string& title)
{
	bool canClose = false;
	ImGui::Begin(title.c_str(), &canClose, window.m_menuWindowFlags);
	float titleWidth = ImGui::CalcTextSize(title.c_str()).x;
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5);
	ImGui::TextUnformatted(title.c_str());
}
void screens::end()
{
	ImGui::End();
}