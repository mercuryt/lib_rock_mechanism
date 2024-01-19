#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class MainMenuView final
{
	Window& m_window;
	tgui::Group::Ptr m_panel;
public:
	MainMenuView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
};
