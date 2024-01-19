#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Group.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
class Window;
class StocksView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::ScrollablePanel::Ptr m_list;
public:
	StocksView(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void draw();
};
