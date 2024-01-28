#pragma once
#include "widgets.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
class Window;
class Simulation;
class EditRealityView final
{
	Window& m_window;
	tgui::Group::Ptr m_panel;
	tgui::Label::Ptr m_title;
	tgui::EditBox::Ptr m_name;
	DateTimeUI m_dateTime;
	AreaSelectUI m_areaSelect;
	tgui::VerticalLayout::Ptr m_areaHolder;
	tgui::Button::Ptr m_confirm;
	tgui::Button::Ptr m_areaEdit;
	tgui::Button::Ptr m_areaView;
public:
	EditRealityView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void draw();
};
