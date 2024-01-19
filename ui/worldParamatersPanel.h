#pragma once

// Parameters for creating a world.

#include <TGUI/TGUI.hpp>
class Window;
class WorldParamatersView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
public:
	WorldParamatersView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
};
