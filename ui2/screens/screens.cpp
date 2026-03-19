#include "screens.h"
#include "../window.h"
#include "../displayData.h"
void screens::begin(Window& window, const std::string& title, const ImVec2* windowSize)
{
	ImGuiIO& io = ImGui::GetIO();
	bool canClose = false;
	if(windowSize == nullptr)
		ImGui::SetNextWindowSize({io.DisplaySize.x, 0.0f});
	else
		ImGui::SetNextWindowSize(*windowSize);
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin(title.c_str(), &canClose, window.m_menuWindowFlags);
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	float titleWidth = ImGui::CalcTextSize(title.c_str()).x;
	ImGui::SetCursorPosX((io.DisplaySize.x - titleWidth) * 0.5f);
	ImGui::TextUnformatted(title.c_str());
	ImGui::Dummy(ImVec2(0, 20)); // spacing
	ImGui::PopFont();
}
void screens::end()
{
	ImGui::End();
}