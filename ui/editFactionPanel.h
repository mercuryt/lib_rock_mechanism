#pragma once
#include <TGUI/TGUI.hpp>
struct Faction;
class Simulation;
class Window;
class EditFactionView
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
public:
	EditFactionView(Window& window);
	void draw(const FactionId& faction);
	void hide() { m_panel->setVisible(false); }
};
class EditFactionsView
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
public:
	EditFactionsView(Window& window);
	void draw();
	void hide() { m_panel->setVisible(false); }
};
