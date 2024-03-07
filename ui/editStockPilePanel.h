#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class StockPile;
class EditStockPileView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	StockPile* m_stockPile;
public:
	EditStockPileView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void draw(StockPile* stockPile = nullptr);
};
