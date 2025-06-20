#pragma once
# include "../engine/numericTypes/types.h"
#include <TGUI/TGUI.hpp>
class Window;
class Area;
struct Faction;
struct CraftJobType;
struct MaterialType;
class ProductionView;
struct ItemType;
struct MaterialType;
struct CraftJob;
class NewProductionView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	ProductionView& m_productionView;
	Quantity m_quantity = Quantity::create(1);
public:
	NewProductionView(Window& w, ProductionView& pv);
	void draw();
	void hide();
	friend class ProductionView;
};
class ProductionView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	NewProductionView m_newProductionView;
public:
	ProductionView(Window& w);
	void draw();
	void hide();
	void closeCreate() { m_newProductionView.hide(); }
	[[nodiscard]] bool createIsVisible() const { return m_newProductionView.m_panel != nullptr; }
	[[nodiscard]] bool isVisible() const { return m_panel != nullptr; }
	[[nodiscard]] std::vector<std::tuple<CraftJobTypeId, MaterialTypeId, Quantity, CraftJob*>> collate();
};
