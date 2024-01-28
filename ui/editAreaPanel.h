#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class Area;
class EditAreaView final
{
	Window& m_window;
	tgui::Group::Ptr m_panel;
	tgui::Label::Ptr m_title;
	tgui::EditBox::Ptr m_name;
	tgui::Container::Ptr m_dimensionsHolder;
	tgui::SpinControl::Ptr m_sizeX;
	tgui::SpinControl::Ptr m_sizeY;
	tgui::SpinControl::Ptr m_sizeZ;
	Area* m_area;
public:
	EditAreaView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void draw(Area* area);
	void confirm();
};
