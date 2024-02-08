#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class LoadView final
{
	tgui::Panel::Ptr m_panel;
	Window& m_window;
public:
	LoadView(Window& w) : m_window(w) { }
	void draw();
	void close();
};
