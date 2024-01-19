#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class LoadView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
public:
	LoadView(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
};
