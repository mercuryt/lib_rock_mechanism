#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class Area;
struct Faction;
struct CraftJobType;
struct MaterialType;
class ProductionView;
class NewProductionView final
{
	Window& m_window;
	ProductionView& m_productionView;
	tgui::Group::Ptr m_group;
	tgui::ComboBox::Ptr m_craftJobTypeSelector;
	tgui::ComboBox::Ptr m_materialTypeSelector;
	tgui::SpinControl::Ptr m_quantitySelector;
	const CraftJobType* m_craftJobType;
	const MaterialType* m_materalType;
	uint32_t m_quantity;
public:
	NewProductionView(Window& w, ProductionView& productionView);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void display();
};
class ProductionView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Grid::Ptr m_list;
	NewProductionView m_newProductionView;
	Area* m_area;
public:
	ProductionView(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void display();
};
