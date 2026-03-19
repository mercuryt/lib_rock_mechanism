#pragma once
#include "window.h"

namespace drawInfoPopUp
{
	void actor(Window& window);
	void item(Window& window);
	void plant(Window& window);
	void point(Window& window);
	void begin(const std::string& title);
	void end(Window& window);
	constexpr static ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
}