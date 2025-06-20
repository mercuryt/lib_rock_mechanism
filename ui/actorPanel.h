#pragma once
#include "../engine/numericTypes/index.h"
#include <TGUI/TGUI.hpp>
class Window;
class ActorView final
{
	Window& m_window;
	tgui::ScrollablePanel::Ptr m_panel;
	ActorIndex m_actor;
public:
	ActorView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void draw(const ActorIndex& actor);
};
