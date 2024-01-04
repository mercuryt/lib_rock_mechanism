#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Panel.hpp>
class Block;
class Item;
class Actor;
class Window;
class Plant;

class DetailPanel final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	tgui::Label::Ptr m_name;
	tgui::Label::Ptr m_description;
	tgui::Group::Ptr m_contents;
	tgui::Button::Ptr m_more;
	tgui::Button::Ptr m_exit;
public:
	DetailPanel(Window& window, tgui::Group::Ptr gameGui);
	void display(Block& actor);
	void display(Item& actor);
	void display(Plant& plant);
	void display(Actor& actor);
	void hide() { m_panel->setVisible(false); }
	bool isVisible() { return m_panel->isVisible(); }
};
