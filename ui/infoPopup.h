#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Panel.hpp>
class BlockIndex;
class Item;
class Actor;
class Window;
class Plant;

class InfoPopup final
{
	Window& m_window;
	tgui::Grid::Ptr m_grid;
	tgui::ChildWindow::Ptr m_childWindow;
	std::function<void()> m_update;
	uint32_t m_count = 0;
public:
	InfoPopup(Window& window);
	void makeWindow();
	void add(tgui::Widget::Ptr widget);
	void display(BlockIndex& block);
	void display(Item& item);
	void display(Plant& plant);
	void display(Actor& actor);
	void update() { assert(m_update); m_update(); }
	void hide();
	bool isVisible() const;
};
