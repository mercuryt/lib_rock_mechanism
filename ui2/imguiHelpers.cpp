#include"imguiHelpers.h"
#include "imguiStdString.h"
bool imguiButtonCentered(const std::string label)
{
	float windowSizeX = ImGui::GetWindowSize().x;
	float buttonWidth = ImGui::CalcTextSize(label.c_str()).x + 20.f;  // Padding around text
	ImVec2 buttonSize(buttonWidth, 0);  // Height will auto-calculate based on the label
	ImVec2 buttonPos((windowSizeX - buttonWidth) * 0.5f, ImGui::GetCursorPosY());
	ImGui::SetCursorPos(buttonPos);
	return ImGui::Button(label.c_str(), buttonSize);
}