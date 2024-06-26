#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class Actor;
class ActorView final
{
	Window& m_window;
	tgui::ScrollablePanel::Ptr m_panel;
	Actor* m_actor;
public:
	ActorView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void draw(Actor& actor);
};
