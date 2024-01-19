#pragma once
#include "../engine/objective.h"
#include <TGUI/TGUI.hpp>
class Window;
class Actor;
class ObjectivePriorityView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Grid::Ptr m_grid;
	tgui::Label::Ptr m_actorName;
	Actor* m_actor;
	std::map<ObjectiveTypeId, tgui::SpinButton::Ptr> m_spinButtons;
public:
	ObjectivePriorityView(Window& w);
	void draw(Actor& actor);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
};
