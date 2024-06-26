#pragma once
#include "widgets.h"
#include <TGUI/TGUI.hpp>
class Window;
class Actor;
class EditActorView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	Actor* m_actor;
public:
	EditActorView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void draw(Actor& actor);
};
