#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Panel.hpp>
class Block;
class Item;
class Actor;
class Window;
class Plant;

class InfoPopup final
{
	Window& m_window;
	tgui::Grid::Ptr m_grid;
	tgui::ChildWindow::Ptr m_childWindow;
	uint32_t m_count = 0;
public:
	InfoPopup(Window& window);
	void makeWindow();
	void add(tgui::Widget::Ptr widget);
	void display(Block& block);
	void display(Item& item);
	void display(Plant& plant);
	void display(Actor& actor);
	void hide() { m_childWindow->setVisible(false); }
	bool isVisible() const { return m_childWindow->isVisible(); }
};