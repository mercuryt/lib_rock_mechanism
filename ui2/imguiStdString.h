#pragma once
#include <string>
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
#include "imgui/imgui.h"
_Pragma("GCC diagnostic pop")
inline void ImGuiText(const std::string& text)
{
	ImGui::TextUnformatted(text.c_str());
}
inline void ImGuiLabelText(const char* label, const std::string& value)
{
	ImGui::LabelText(label, "%s", value.c_str());
}
inline void ImGuiBeginCombo(const char* label, const std::string& value)
{
	ImGui::BeginCombo(label, value.c_str());
}
inline void ImGuiTextCentered(const std::string& text)
{
	float textWidth = ImGui::CalcTextSize(text.c_str()).x;
	float regionWidth = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX((regionWidth - textWidth) * 0.5f);
	ImGui::TextUnformatted(text.c_str());
}
[[nodiscard]] inline bool ImGuiSelectable(const std::string& label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2& size = ImVec2(0,0))
{
	return ImGui::Selectable(label.c_str(), selected, flags, size);
}
[[nodiscard]] inline bool ImGuiButton(const std::string& label, const ImVec2& size = ImVec2(0,0))
{
	return ImGui::Button(label.c_str(), size);
}
[[nodiscard]] inline ImVec2 ImGuiCalcTextSize(const std::string& text, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)
{
	return ImGui::CalcTextSize(text.c_str(), nullptr, hide_text_after_double_hash, wrap_width);
}