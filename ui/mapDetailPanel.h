#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Panel.hpp>
class Window;
struct WorldLocation;

class DetailPanel final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	tgui::Label::Ptr m_description;
	tgui::Button::Ptr m_exit;
	tgui::Button::Ptr m_confirm;
public:
	DetailPanel(Window& window, tgui::Group::Ptr gameGui);
	void display(WorldLocation& worldLocation);
	void hide() { m_panel->setVisible(false); }
	bool isVisible() const { return m_panel->isVisible(); }
};
