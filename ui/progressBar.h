#pragma once
class Window;
#include <TGUI/TGUI.hpp>
#include <string>

class ProgressBar
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	std::wstring m_title;
	Percent m_percent;
public:
	ProgressBar(Window& window);
	void draw();
	void hide() { m_panel->setVisible(false); }
	void setTitle(const std::wstring& title) { m_title = title; }
};