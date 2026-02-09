#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class MainMenuView final
{
	Window& m_window;
	tgui::Group::Ptr m_panel;
	void update();
public:
	MainMenuView(Window& w);
	void show() { update(); m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
};
