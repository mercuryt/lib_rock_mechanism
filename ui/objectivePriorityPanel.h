#pragma once
#include "../engine/objective.h"
#include <TGUI/TGUI.hpp>
class Window;
class Actor;
class ObjectivePriorityView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	tgui::Label::Ptr m_title;
	tgui::Grid::Ptr m_grid;
	std::map<ObjectiveTypeId, tgui::SpinControl::Ptr> m_spinControls;
	Actor* m_actor;
public:
	ObjectivePriorityView(Window& w);
	void draw(Actor& actor);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
};