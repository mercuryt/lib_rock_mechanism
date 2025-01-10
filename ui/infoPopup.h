#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Panel.hpp>
class BlockIndex;
class Window;

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
	void display(const ItemIndex& item);
	void display(const PlantIndex& plant);
	void display(const ActorIndex& actor);
	void update() { assert(m_update); m_update(); }
	void hide();
	bool isVisible() const;
};
